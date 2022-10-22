#include "SFML/Graphics.hpp"
#include "../imgui/imgui.h"
#include "../imgui/imgui-SFML.h"

#include <iostream>

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

struct Node
{
    // init tile w/ default values:
    Node(
        const sf::Vector2f& size = { 25.f, 25.f },
        const sf::Color& colour = sf::Color::Blue,
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

    /* TODO: */
    // @parent node
    // @neighbour nodes
    // @hcost, fcost, gcost etc.
};

void HandleTileClick(
    Node nodes[][mapHeight], 
    sf::Vector2f& mpos,
    const sf::Color& colour = sf::Color::Black)
{
    for (int i = 0; i < mapWidth; i++)
    {
        for (int j = 0; j < mapHeight; j++)
        {
            if (nodes[i][j].tile
                .getGlobalBounds().contains(mpos))
            {
                nodes[i][j].tile
                    .setFillColor(colour);
            }
        }
    }
}

/* TODO: */
// heap allocate... then map 2d array to 1d.
int main()
{
    // sfml + imgui window inits:
    sf::RenderWindow window(
        sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT),
        "A* Pathfinding Algorithm");
    ImGui::SFML::Init(window);

    // grid of nodes
    Node nodes[mapWidth][mapHeight];

    // hold current mouse coordinates:
    sf::Vector2f mpos;

    float col = 10.f; 
    // setup tile position as grid:
    for (int i = 0; i < mapWidth; i++)
    {
        float row = 10.f;
        for (int j = 0; j < mapHeight; j++)
        {
            nodes[i][j].tile
                .setPosition(row, col);
            row += 28.f;
        }
        col += 28.f;
    }

    sf::Clock dt;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            ImGui::SFML::ProcessEvent(event);
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::MouseButtonPressed:
                switch (event.mouseButton.button)
                {
                case sf::Mouse::Left:
                    if (sf::Keyboard::isKeyPressed(
                        sf::Keyboard::S))
                    {
                        std::cout << "S pressed!\n";
                        startKeyDown = true;
                        // colour start node:
                        // ...
                    }
                    if (sf::Keyboard::isKeyPressed(
                        sf::Keyboard::E))
                    {
                        std::cout << "E pressed!\n";
                        endKeyDown = true;
                        // colour end node:
                        // ...
                    }
                    mouseLeftDown = true;
                    break;
                
                case sf::Mouse::Right:
                    mouseRightDown = true;
                    break;
                }
                break;

            case sf::Event::MouseMoved:
                // current mouse position:
                mpos = window
                    .mapPixelToCoords(
                        sf::Mouse::getPosition(window));
                break;
            
            case sf::Event::MouseButtonReleased:
                switch (event.mouseButton.button)
                {
                case sf::Mouse::Left:
                    if (sf::Keyboard::isKeyPressed(
                        sf::Keyboard::S))
                    {
                        std::cout << "S released!\n";
                        startKeyDown = true;
                        // colour start node:
                        // ...
                    }
                    if (sf::Keyboard::isKeyPressed(
                        sf::Keyboard::E))
                    {
                        std::cout << "E released!\n";
                        endKeyDown = true;
                        // colour end node:
                        // ...
                    }
                    mouseLeftDown = false;
                    break;

                case sf::Mouse::Right:
                    mouseRightDown = false;
                    break;
                }
                break;
            }
        }

        ImGui::SFML::Update(
            window, dt.restart());

        /*if (mouseLeftDown)
        {
            HandleTileClick(nodes, mpos);
        }*/

        // draw walls:
        if (mouseRightDown)
        {
            HandleTileClick(nodes, mpos);
        }

        /* imgui stuff: */
        ImGui::Begin("Menu");
        if (ImGui::Button("clear"))
        {
            for (auto& row : nodes)
                for (auto& col : row)
                    col.tile.setFillColor(sf::Color::Blue);
        }
        ImGui::End();

        window.clear(sf::Color::White);
        
        // display grid:
        for (int i = 0; i < mapWidth; i++)
            for (int j = 0; j < mapHeight; j++)
                window.draw(nodes[i][j].tile);
     
        ImGui::SFML::Render(window);
        window.display();
    }
    ImGui::SFML::Shutdown();
}