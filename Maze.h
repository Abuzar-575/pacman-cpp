#pragma once

#ifndef MAZE_H
#define MAZE_H

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace sf;

class Maze {
public:
    // Singleton instance
    static Maze* instance;

    // Maze pattern (layout)
    const string mazePattern[25] = {
        "#######################", // 0   // #= Bricks
        "#Q   S    W#Q    S   W#", // 1   // X= Ghosts direction condition
        "#@### #### # #### ###@#", // 2   // c= Pacman char
        "# ### #### # #### ### #", // 3   // Q=RD
        "#D   X S  A A  S X   F#", // 4   // W=LD
        "# ### # ####### # ### #", // 5   // E=RU
        "#E   F# ####### #D   R#", // 6   // R=LU
        "##### #E  W#Q  R# #####", // 7   //-------
        "##### #### # #### #####", // 8   // A=LRU 
        "##### #Q  AGA  W# #####", // 9   // S=LRD
        "##### # ###-### # #####", // 12  // D=UDR
        "0====X F#BP*IC#D X====0", // 11  // F=UDL
        "##### # ####### # #####", // 12  // G=LR   
        "##### #D0001000F# #####", // 13  // 1=Fruit location
        "##### # ####### # #####", // 14  // L=Low speed in tunel for ghosts
        "##### # ####### # #####", // 15  // BPIC=Ghosts char
        "#Q   X A  W#Q  A X   W#", // 16
        "# ### #### # #### ### #", // 17
        "#E W#D S  AcA  S F#Q R#", // 18
        "### # # ####### # # ###", // 19
        "#Q A R#E  W#Q  R#E A W#", // 20
        "# ######## # ######## #", // 24
        "#@######## # ########@#", // 22
        "#E        A A        R#", // 23
        "#######################" // 24
    };
    bool isGhostHouse(int x, int y) const {
        string cell = getCellAt(x, y);
        // Check for ghost house markers: *, B, P, I, C, -, etc.
        return (cell == "*" || cell == "B" || cell == "P" ||
            cell == "I" || cell == "C" || cell == "-");
    }

    // Grid to store maze elements
    string mazeGrid[23][25];

    // Dots storage
    vector<CircleShape> dotsArr;

    // Wall lines storage
    vector<RectangleShape> wallLines;
    int getHeight() const {
        return mazeH;
    }
    int getWidth() const {
        return mazeW;
    }
    // Score tracking (just the value, no display)
    int score = 0;
    int level = 1;

    // Constants
    static const int mazeW = 23;
    static const int mazeH = 25;
    static const int cellSize = 28;  // Size of each cell in pixels
    static const int wallThickness = 2;  // Thickness of wall lines

    // Constructor
    Maze() {
        cout << "Maze class OK" << endl;
        instance = this;

        // Initialize the maze
        initMaze();
    }

    // Destructor
    ~Maze() {
        // Clean up if needed
    }

    // Initialize the maze from pattern
    void initMaze() {
        int colum = 0;
        int row = 0;

        dotsArr.clear();
        wallLines.clear();

        // First, build the grid representation
        for (int i = 0; i < (mazeW * mazeH); i++) {
            if (colum >= mazeW) {
                colum = 0;
                row++;
            }

            if (row < mazeH) {
                string str = mazePattern[row].substr(colum, 1);
                mazeGrid[colum][row] = str;

                // Place dots where appropriate
                if (isDot(str)) {
                    // Regular dot or energizer
                    dotsArr.push_back(getDots(colum, row, str));
                }
            }

            colum++;
        }

        // Now create wall lines based on the grid
        createWallLines();
    }

    // Create wall outline lines for the maze
    void createWallLines() {
        for (int y = 0; y < mazeH; y++) {
            for (int x = 0; x < mazeW; x++) {
                if (mazeGrid[x][y] == "#") {
                    // This is a wall cell

                    // Check adjacent cells to determine which walls to draw

                    // Top wall
                    if (y == 0 || mazeGrid[x][y - 1] != "#") {
                        RectangleShape line;
                        line.setSize(Vector2f(cellSize, wallThickness));
                        line.setPosition(Vector2f(x * cellSize, y * cellSize));
                        line.setFillColor(Color::Blue);
                        wallLines.push_back(line);
                    }

                    // Bottom wall
                    if (y == mazeH - 1 || mazeGrid[x][y + 1] != "#") {
                        RectangleShape line;
                        line.setSize(Vector2f(cellSize, wallThickness));
                        line.setPosition(Vector2f(x * cellSize, (y + 1) * cellSize - wallThickness));
                        line.setFillColor(Color::Blue);
                        wallLines.push_back(line);
                    }

                    // Left wall
                    if (x == 0 || mazeGrid[x - 1][y] != "#") {
                        RectangleShape line;
                        line.setSize(Vector2f(wallThickness, cellSize));
                        line.setPosition(Vector2f(x * cellSize, y * cellSize));
                        line.setFillColor(Color::Blue);
                        wallLines.push_back(line);
                    }

                    // Right wall
                    if (x == mazeW - 1 || mazeGrid[x + 1][y] != "#") {
                        RectangleShape line;
                        line.setSize(Vector2f(wallThickness, cellSize));
                        line.setPosition(Vector2f((x + 1) * cellSize - wallThickness, y * cellSize));
                        line.setFillColor(Color::Blue);
                        wallLines.push_back(line);
                    }
                }
            }
        }
    }

    // Draw the maze to the window
    void draw(RenderWindow& window) {
        // Draw wall lines
        for (size_t i = 0; i < wallLines.size(); i++) {
            window.draw(wallLines[i]);
        }

        // Draw dots
        for (size_t i = 0; i < dotsArr.size(); i++) {
            window.draw(dotsArr[i]);
        }
    }

    // Update the score (just tracking the value)
    void setScore(int newScore) {
        score = newScore;
    }

    // Update the level (just tracking the value)
    void setLevel(int newLevel) {
        level = newLevel;
    }

    // Redraw dots (make them visible again)
    void redrawDots() {
        // Reset the eaten status of all dots
        for (int x = 0; x < mazeW; x++) {
            for (int y = 0; y < mazeH; y++) {
                dotsEaten[x][y] = false;
            }
        }

        // Make all dots visible again
        for (size_t i = 0; i < dotsArr.size(); i++) {
            dotsArr[i].setFillColor(Color::Yellow);
        }
    }

    // Get the cell content at specific position
    string getCellAt(int x, int y) const {
        if (x >= 0 && x < mazeW && y >= 0 && y < mazeH) {
            return mazeGrid[x][y];
        }
        return "#"; // Default to wall for out of bounds
    }

    // Check if a position is a wall
    bool isWall(int x, int y) const {
        return getCellAt(x, y) == "#";
    }

    // Check if a position has a dot
    bool hasDot(int x, int y) const {
        string cell = getCellAt(x, y);
        return isDot(cell) && !dotsEaten[x][y];
    }

    // Convert grid coordinates to screen coordinates
    Vector2f gridToScreen(int x, int y) const {
        return Vector2f(x * cellSize + cellSize / 2, y * cellSize + cellSize / 2);
    }

    // Convert screen coordinates to grid coordinates
    Vector2i screenToGrid(float x, float y) const {
        return Vector2i(static_cast<int>(x / cellSize),
            static_cast<int>(y / cellSize));
    }

    // Remove a dot at specified position
    void removeDot(int x, int y) {
        // If we've already eaten this dot, don't do anything
        if (dotsEaten[x][y]) {
            return;
        }

        // Mark this dot as eaten
        dotsEaten[x][y] = true;

        // Find the dot at this position and make it invisible
        Vector2f pos = gridToScreen(x, y);

        for (size_t i = 0; i < dotsArr.size(); i++) {
            float dotX = dotsArr[i].getPosition().x + dotsArr[i].getRadius();
            float dotY = dotsArr[i].getPosition().y + dotsArr[i].getRadius();

            if (abs(dotX - pos.x) < cellSize / 2 && abs(dotY - pos.y) < cellSize / 2) {
                // Found the dot, make it invisible
                dotsArr[i].setFillColor(Color::Transparent);

                // Add to score (power pellets worth more)
                if (dotsArr[i].getRadius() > 4) {
                    setScore(score + 50);  // Power pellet
                }
                else {
                    setScore(score + 10);  // Regular dot
                }
                break;
            }
        }
    }

private:
    bool dotsEaten[mazeW][mazeH] = { false };
    // Check if a cell contains a dot
    bool isDot(string str) const {
        return str != "0" && str != "=" &&
            str != "B" && str != "P" &&
            str != "I" && str != "C" &&
            str != "G" && str != "*" &&
            str != "1" && str != "-" && str != "c" &&
            str != "#";  // Also exclude walls
    }

    // Create a dot shape at specified position
    CircleShape getDots(int col, int row, string str) {
        float radius = 2;  // Standard small dot
        Color color = Color::Yellow;

        // Power pellets (energizers) are larger
        if (str == "@") {
            radius = 6;  // Larger for power pellets
        }

        CircleShape dot;
        dot.setRadius(radius);
        dot.setFillColor(color);

        // Center the dot in its cell
        dot.setPosition(Vector2f((col + 0.5f) * cellSize - radius,
            (row + 0.5f) * cellSize - radius));

        return dot;
    }
};

// Initialize the static instance
Maze* Maze::instance = nullptr;

#endif // MAZE_H

