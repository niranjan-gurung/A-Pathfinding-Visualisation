#include "SFML/Graphics.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;

static const int mapWidth = 20;
static const int mapHeight = 20;

/* Forward declare Node struct: */
struct Node;

using Tile = sf::RectangleShape;

// mouse flags:
bool mouseLeftDown = false;
bool mouseRightDown = false;
bool startKeyDown = false;
bool endKeyDown = false;

bool algorithmStart = false;

// sfml + imgui window inits:
sf::RenderWindow window(
    sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT),
    "A* Pathfinding Algorithm");

// grid of nodes:
std::vector<Node> nodes{ mapWidth * mapHeight };

// mouse coords:
sf::Vector2f mpos{};

Node* startNode = nullptr;
Node* endNode = nullptr;

/* Forward Declarations: */
// Inits:
void InitGridConnections();
void HandleTileClick(const sf::Color& colour);

// Main Algorithm:
void AStarAlgorithm();

// A* Path:
void RetracePath();

// Main loop:
void UpdateImGuiContext();
void Update(sf::Clock& dt);
void Render();

int main()
{
    ImGui::SFML::Init(window);

    // setup tile grid:
    InitGridConnections();

    sf::Clock dt;
    while (window.isOpen())
    {
        Update(dt);
        UpdateImGuiContext();  /* imgui menu: */
        Render();
    }
    ImGui::SFML::Shutdown();
}

struct Node
{
    // init tile w/ default values:
    Node(
        const sf::Vector2f& size = { 25.f, 25.f },
        const sf::Color& colour = sf::Color::White,
        const float outlineThickness = 1.f,
        const sf::Color& outlineColour = sf::Color::Black)
    {
        tile.setSize(size);
        tile.setFillColor(colour);
        tile.setOutlineThickness(outlineThickness);
        tile.setOutlineColor(outlineColour);
    }

    // individual grid squares:
    Tile tile;

    // track current node's parent:
    Node* parent = nullptr;

    // surrounding nodes for any given node:
    std::vector<Node*> neighbours;

    // walls:
    bool obstacle = false;
    bool visited = false;

    float gcost = 0.0f;    // distance from start node
    float hcost = 0.0f;    // distance from end node (heuristic)
    float fcost = 0.0f;    // g + h = fcost

    // helper function,
    // return tile x, y coords:
    auto GetTilePosition()
    {
        return sf::Vector2f(
            tile.getPosition().x,
            tile.getPosition().y);
    }
};

void InitGridConnections()
{
    // setup tile position as grid:
    float col = 20.f;
    for (int x = 0; x < mapWidth; x++)
    {
        float row = 20.f;
        for (int y = 0; y < mapHeight; y++)
        {
            // set origin to center of each node:
            nodes[x + mapWidth * y].tile
                .setOrigin(
                    sf::Vector2f(
                        nodes[x + mapWidth * y].tile.getPosition().x +
                        (nodes[x + mapWidth * y].tile.getGlobalBounds().width / 2),
                        nodes[x + mapWidth * y].tile.getPosition().y +
                        (nodes[x + mapWidth * y].tile.getGlobalBounds().height / 2)));

            nodes[x + mapWidth * y].tile
                .setPosition(row, col);
            row += 28.f;
        }
        col += 28.f;
    }

    // create references for current node with neighbours (surrouding nodes):
    for (int x = 0; x < mapWidth; x++)
    {
        for (int y = 0; y < mapHeight; y++)
        {
            // N-E-S-W connections:
            // top node
            if (y > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[x + mapWidth * (y - 1)]);
            // right node
            if (x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * y]);
            // bottom node
            if (y < mapHeight - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[x + mapWidth * (y + 1)]);
            // left node
            if (x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * y]);

            // diagonal connections:
            // top left
            if (y > 0 && x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * (y - 1)]);
            // top right
            if (y > 0 && x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * (y - 1)]);
            // bottom right
            if (y < mapHeight - 1 && x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * (y + 1)]);
            // bottom left
            if (y < mapHeight - 1 && x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * (y + 1)]);
        }
    }
}

void HandleTileClick(const sf::Color& colour = sf::Color::Black)
{
    for (auto& row : nodes)
    {
        // if tile click...
        if (row.tile
            .getGlobalBounds().contains(mpos))
        {
            // set start node:
            if (sf::Keyboard
                ::isKeyPressed(sf::Keyboard::S))
            {
                row.tile
                    .setFillColor(sf::Color::Green);
                startNode = &row;
            }
            // set end node:
            else if (sf::Keyboard
                ::isKeyPressed(sf::Keyboard::E))
            {
                row.tile
                    .setFillColor(sf::Color::Red);
                endNode = &row;
            }
            // remove walls:
            else if (sf::Mouse
                ::isButtonPressed(sf::Mouse::Right))
            {
                row.tile
                    .setFillColor(sf::Color::White);
                row.obstacle = false;
            }
            // set walls:
            else
            {
                row.tile
                    .setFillColor(colour);
                row.obstacle = true;
            }
        }
    }
}

// Path generated by A* algorithm:
void RetracePath()
{
    Node* tracker = endNode;
    while (tracker->parent != nullptr) {
        // continue to update tracker to current node's parent until start node is reached:
        tracker = tracker->parent;

        // only colour in path:
        if (tracker != startNode)
            tracker->tile.setFillColor(sf::Color::Yellow);
    }
}

/* Main Algorithm : */
void AStarAlgorithm()
{
    /* returns distance between any two given tiles.
     * used for calculating g and h costs
     */
    auto distance = [](Node* a, Node* b) -> float
    {
        return sqrtf(
            std::pow(a->GetTilePosition().x - b->GetTilePosition().x, 2) +
            std::pow(a->GetTilePosition().y - b->GetTilePosition().y, 2));
    };

    // list of nodes to test:
    std::vector<Node*> openList{};
    // list of tested nodes:
    std::vector<Node*> closedList{};
    openList.push_back(startNode);

    while (!openList.empty())
    {
        Node* currentNode = openList.front();

        // find lowest fcost node from openlist:
        for (int i = 1; i < openList.size(); i++)
        {
            if (openList[i]->fcost < currentNode->fcost
                || openList[i]->fcost == currentNode->fcost 
                && openList[i]->hcost < currentNode->hcost)
            {
                currentNode = openList[i];
            }
        }
            
        // remove currentNode (lowest fcost node thats searched) from openlist: 
        openList.erase(
            std::remove(
                openList.begin(), openList.end(), currentNode), openList.end());

        // add it to closed list:
        closedList.push_back(currentNode);

        // end goal reached:
        if (currentNode == endNode)
        {
            RetracePath();
            algorithmStart = false;     // stops algorithm
            return;
        }

        /* Search surrouding neighbour nodes of currentNode (startnode by default). 
         * order in which neighbours list is processed (counter clockwise): 
         * left, botom, right, top, top-left, bottom-left, top-right, bottom-right.
         */
        for (auto& currentNeighbour : currentNode->neighbours)
        {
            if (currentNeighbour->obstacle ||
                std::find(
                    closedList.begin(),
                    closedList.end(),
                    currentNeighbour) != closedList.end())
                continue;

            float costToMove =
                currentNode->gcost + distance(currentNode, currentNeighbour);

            if (costToMove < currentNeighbour->gcost || 
                !(std::find(
                    openList.begin(),
                    openList.end(),
                    currentNeighbour) != openList.end()))
            {
                currentNeighbour->parent = currentNode;
                currentNeighbour->gcost = costToMove;
                currentNeighbour->hcost = distance(currentNeighbour, endNode);
                currentNeighbour->fcost = currentNeighbour->gcost + currentNeighbour->hcost;
                
                openList.push_back(currentNeighbour);
                
                // colour in searched nodes:
                if (currentNeighbour != endNode)
                    currentNeighbour->tile.setFillColor(sf::Color::Magenta);
            }

            /*printf("x: %.0f y: %.0f fcost: %.0f\n", 
                currentNeighbour->tile.getPosition().x, 
                currentNeighbour->tile.getPosition().y, 
                currentNeighbour->fcost);*/
        }
    }
}

void UpdateImGuiContext()
{
    ImGui::Begin("Menu");

    if (ImGui::Button("visualise"))
        algorithmStart = true;

    if (ImGui::Button("clear"))
    {
        for (auto& row : nodes)
        {
            startNode = endNode = nullptr;
            row.parent = nullptr;
            row.obstacle = false;
            row.gcost = row.hcost = row.fcost = 0;
            row.tile
                .setFillColor(sf::Color::White);
        }
    }
    ImGui::End();
}

void Update(sf::Clock& dt)
{
    sf::Event event;
    while (window.pollEvent(event))
    {
        ImGui::SFML::ProcessEvent(event);
        switch (event.type)
        {
            // close window:
        case sf::Event::Closed:
            window.close();
            break;

            // key events for choosing start + end tile:
        case sf::Event::KeyPressed:
            switch (event.key.code)
            {
            case sf::Keyboard::S:
                startKeyDown = true;
                break;

            case sf::Keyboard::E:
                endKeyDown = true;
                break;
            }
            break;

        case sf::Event::KeyReleased:
            switch (event.key.code)
            {
            case sf::Keyboard::S:
                startKeyDown = false;
                break;

            case sf::Keyboard::E:
                endKeyDown = false;
                break;
            }
            break;

            // Mouse events:
        case sf::Event::MouseButtonPressed:
            switch (event.mouseButton.button)
            {
            case sf::Mouse::Left:
                mouseLeftDown = true;
                break;

            case sf::Mouse::Right:
                mouseRightDown = true;
                break;
            }
            break;

        case sf::Event::MouseButtonReleased:
            switch (event.mouseButton.button)
            {
            case sf::Mouse::Left:
                mouseLeftDown = false;
                break;

            case sf::Mouse::Right:
                mouseRightDown = false;
                break;
            }
            break;

        case sf::Event::MouseMoved:
            // current mouse position:
            mpos = window
                .mapPixelToCoords(
                    sf::Mouse::getPosition(window));
            break;
        }
    }

    /* Update */
    ImGui::SFML::Update(
        window, dt.restart());

    // start/end node:
    if (mouseLeftDown &&
        (startKeyDown || endKeyDown))
        HandleTileClick();

    // draw walls:
    if (mouseLeftDown)
        HandleTileClick();

    // remove walls:
    if (mouseRightDown)
        HandleTileClick();

    // A* visualisation..
    if (algorithmStart)
        AStarAlgorithm();
}

void Render()
{
    /* Render */
    window.clear(sf::Color::Blue);

    // display grid:
    for (const auto& row : nodes)
        window.draw(row.tile);

    ImGui::SFML::Render(window);
    window.display();
}