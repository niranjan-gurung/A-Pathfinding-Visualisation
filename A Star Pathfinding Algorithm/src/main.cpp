#include "SFML/Graphics.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"

#include <iostream>
#include <vector>
#include <cmath>

static const int SCREEN_WIDTH = 800;
static const int SCREEN_HEIGHT = 600;

static const int mapWidth = 20;
static const int mapHeight = 20;

using Tile = sf::RectangleShape;

// mouse flags:
bool mouseLeftDown = false;
bool mouseRightDown = false;
bool startKeyDown = false;
bool endKeyDown = false;

// represents the distance between each node.
// set as 1... but can be assumed as any value:
const float nodeDistance = 1.0f;
const float nodeDiagonalDistance = 1.4f;

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

    Node* parent = nullptr;

    // surrounding nodes for any given node
    std::vector<Node*> neighbours;

    // if a node has been visited during a search
    bool visited = false;

    float gcost = 0.0f;    // distance from start node
    float hcost = 0.0f;    // distance from end node (heuristic)
    float fcost = 0.0f;    // g + h = fcost
    
    // helper function
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
            // bottom node
            if (y < mapHeight - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[x + mapWidth * (y + 1)]);
            // left node
            if (x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * y]);
            // right node
            if (x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * y]);

            // diagonal connections:
            // top left
            if (y > 0 && x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * (y - 1)]);
            // bottom left
            if (y < mapHeight - 1 && x > 0)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x - 1) + mapWidth * (y + 1)]);
            // top right
            if (y > 0 && x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * (y - 1)]);
            // bottom right
            if (y < mapHeight - 1 && x < mapWidth - 1)
                nodes[x + mapWidth * y]
                    .neighbours.push_back(&nodes[(x + 1) + mapWidth * (y + 1)]);
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

/* Main Algorithm : */
void AStarAlgorithm(std::vector<Node>& nodes)
{
    // Default state for each node,
    // reset after each solve:
    for (int x = 0; x < mapWidth; x++)
    {
        for (int y = 0; y < mapHeight; y++)
        {
            nodes[x + mapWidth * y].visited = false;
            nodes[x + mapWidth * y].parent = nullptr;
            nodes[x + mapWidth * y].gcost = 0.0f;
            nodes[x + mapWidth * y].hcost = 0.0f;
            nodes[x + mapWidth * y].fcost = 0.0f;
        }
    }

    auto distance = [](Node* a, Node* b)
    {
        return sqrtf(
            (a->GetTilePosition().x - b->GetTilePosition().x) * (a->GetTilePosition().x - b->GetTilePosition().x) + 
            (a->GetTilePosition().y - b->GetTilePosition().y) * (a->GetTilePosition().y - b->GetTilePosition().y));
    };

    std::cout << distance(startNode, endNode) << std::endl;

    // calculate gcost and hcost of startnode's immediate neighbours:
    Node* currentNode = startNode;
    currentNode->hcost = 0.0f;  // heuristic
    
    for (int i = 0; i < currentNode->neighbours.size(); i++)    
    {
        // N-E-S-W:
        if (i < 4)
            currentNode->neighbours[i]->gcost = nodeDistance * 10.0f;
        // Diagonals:
        else 
            currentNode->neighbours[i]->gcost = nodeDiagonalDistance * 10.0f;
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

        /* both start and end node has to be set for algorithm to execute,
         * and path to be drawn */
        //if (startNode && endNode)
        //{
        //    Node* tracker = endNode;
        //    while (tracker->parent)
        //    {
        //        // continue to update tracker to current node's parent until start node is reached:
        //        tracker = tracker->parent;
 
        //        // colour in found path in different colour... yellow?
        //        tracker->tile.setFillColor(sf::Color::Yellow);
        //    }
        //}

        /* imgui stuff: */
        ImGui::Begin("Menu");
        if (ImGui::Button("visualise"))
        {
            // A* visualisation..
            AStarAlgorithm(nodes);
        }

        if (ImGui::Button("clear"))
        {
            for (auto& row : nodes)
                row.tile
                    .setFillColor(sf::Color::White);
        }
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