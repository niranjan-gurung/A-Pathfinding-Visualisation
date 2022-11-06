#include "SFML/Graphics.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"

#include <iostream>
#include <iomanip>
#include <vector>

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;

static const int mapWidth = 20;
static const int mapHeight = 20;

using Tile = sf::RectangleShape;

bool algorithmStart = false;

// mouse flags:
bool mouseLeftDown = false;
bool mouseRightDown = false;
bool startKeyDown = false;
bool endKeyDown = false;

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

    // if a node has been visited during a search:
    bool visited = false;
    
    // walls:
    bool obstacle = false;

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

Node* startNode = nullptr;
Node* endNode = nullptr;

void InitGridConnections(std::vector<Node>& nodes)
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

void HandleTileClick(
    std::vector<Node>& nodes, 
    const sf::Vector2f& mpos,
    const sf::Color& colour = sf::Color::Black)
{
    for (auto& row : nodes)
    {
        // if tile click...
        if (row.tile
            .getGlobalBounds().contains(mpos))
        {
            if (sf::Keyboard
                    ::isKeyPressed(sf::Keyboard::S))
            {
                row.tile
                    .setFillColor(sf::Color::Green);
                startNode = &row;
            }
            else if (sf::Keyboard
                    ::isKeyPressed(sf::Keyboard::E))
            {
                row.tile
                    .setFillColor(sf::Color::Red);
                endNode = &row;
            }
            else if (sf::Mouse
                    ::isButtonPressed(sf::Mouse::Right))
            {
                row.tile
                    .setFillColor(sf::Color::White);
            }
            else
            {
                row.tile
                    .setFillColor(colour);
            }
        }
    }
}

void RetracePath(Node* start, Node* end)
{
    Node* tracker = endNode;
    while (tracker->parent != nullptr)
    {
        // continue to update tracker to current node's parent until start node is reached:
        tracker = tracker->parent;
        // colour in found path in different colour... yellow?
        tracker->tile.setFillColor(sf::Color::Yellow);
    }
}

/* Main Algorithm : */
void AStarAlgorithm()
{
    // returns distance between any two given tiles:
    auto distance = [](Node* a, Node* b) -> float
    {
        return sqrtf(
            (a->GetTilePosition().x - b->GetTilePosition().x) * (a->GetTilePosition().x - b->GetTilePosition().x) + 
            (a->GetTilePosition().y - b->GetTilePosition().y) * (a->GetTilePosition().y - b->GetTilePosition().y));
    };

    // determine distance between current node's neighbour and endnode:
    // (same as distance function basically..)
    auto heuristic = [distance](Node* a, Node* b) -> float
    {
        return distance(a, b);
    };

    // list of nodes to test:
    std::vector<Node*> openList{};
    // list of tested nodes:
    std::vector<Node*> closedList{};
    openList.push_back(startNode);

    while (!openList.empty())
    {
        std::cout << "inside while...\n";
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

        // remove searched node from openlist
        // place them in closed list:
        openList.pop_back();
        closedList.push_back(currentNode);

        // end goal reached:
        if (currentNode == endNode)
        {
            std::cout << "found path!\n";
            RetracePath(startNode, endNode);
            algorithmStart = false;
            return;
        }

        /* set gcost for surrounding neighbours of currentNode(startnode by default):
         * neighbour list order is: top, right, down, left, top-left, bottom-left, top-right, bottom-right
         */
        for (auto& currentNeighbour : currentNode->neighbours)
        {
            std::cout << "inside for...\n";

            if (std::find(
                    closedList.begin(), 
                    closedList.end(), 
                    currentNeighbour) != closedList.end())
                continue;

            float costToMove = 
                currentNode->gcost + distance(currentNode, currentNeighbour);

            if (!(std::find(
                    openList.begin(),
                    openList.end(),
                    currentNeighbour) != openList.end()))
                openList.push_back(currentNeighbour);
            else if (costToMove >= currentNeighbour->gcost)
                continue;
            
            currentNeighbour->parent = currentNode;
            currentNeighbour->gcost = costToMove;
            currentNeighbour->hcost = heuristic(currentNeighbour, endNode);
            currentNeighbour->fcost = currentNeighbour->gcost + currentNeighbour->hcost;
        }
    }
}

int main()
{
    // sfml + imgui window inits:
    sf::RenderWindow window(
        sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT),
        "A* Pathfinding Algorithm");
    ImGui::SFML::Init(window);

    // grid of nodes
    std::vector<Node> nodes(mapWidth * mapHeight);

    // hold current mouse coordinates:
    sf::Vector2f mpos;

    InitGridConnections(nodes);

    sf::Clock dt;
    while (window.isOpen())
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
            HandleTileClick(nodes, mpos);

        // draw walls:
        if (mouseLeftDown)
            HandleTileClick(nodes, mpos);

        // remove walls:
        if (mouseRightDown)
            HandleTileClick(nodes, mpos);

        // A* visualisation..
        if (algorithmStart)
            AStarAlgorithm();

        /* imgui stuff: */
        ImGui::Begin("Menu");

        if (ImGui::Button("visualise"))
            algorithmStart = true;

        if (ImGui::Button("clear"))
            for (auto& row : nodes)
                row.tile
                    .setFillColor(sf::Color::White);
        ImGui::End();

        /* Render */
        window.clear(sf::Color::Blue);
        
        // display grid:
        for (auto& row : nodes)
            window.draw(row.tile);
     
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
}