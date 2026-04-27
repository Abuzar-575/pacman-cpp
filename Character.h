#ifndef CHARACTER_H
#define CHARACTER_H

#include <SFML/Graphics.hpp>
#include "maze.h"
#include <iostream>
#include <string>
#include <array>

using namespace std;
using namespace sf;

class Character {
public:
    // Direction enum
    enum Direction {
        NONE = 0,
        RIGHT = 1,
        LEFT = 2,
        UP = 3,
        DOWN = 4
    };

    // Constructor
    Character(Maze* mazeRef) {
        maze = mazeRef;
        bodyRadius = Maze::cellSize / 2.0f - 2.0f;
        speed = DEFAULT_SPEED;
        active = true;
    }

    // Virtual destructor for proper inheritance
    virtual ~Character() {}

    // Update character state (called each frame) - will be overridden by derived classes
    virtual void update(float deltaTime) {
        // Base update functionality
        updateAnimation(deltaTime);
        updateMovement(deltaTime);
    }

    // Draw character to the window - will be overridden by derived classes
    virtual void draw(RenderWindow& window) = 0; // Pure virtual - must be implemented by derived classes

    // Reset character to starting position
    virtual void reset() {
        findStartPosition();
        currentDirection = NONE;
        nextDirection = NONE;
        offsetX = 0.0f;
        offsetY = 0.0f;
        updateSpritePosition();
        active = true;
    }

    // Get X position (grid coordinates)
    int getX() const {
        return gridX;
    }

    // Get Y position (grid coordinates)
    int getY() const {
        return gridY;
    }

    // Check if a move in the given direction is valid
    virtual bool canMove(Direction dir) {
        int nextX = gridX;
        int nextY = gridY;

        // Calculate the target cell based on direction
        switch (dir) {
        case RIGHT: nextX++; break;
        case LEFT: nextX--; break;
        case UP: nextY--; break;
        case DOWN: nextY++; break;
        default: break;
        }

        // Check for tunnel wrap-around
        if (nextX < 0) nextX = Maze::mazeW - 1;
        if (nextX >= Maze::mazeW) nextX = 0;

        // Check if the target cell is a wall
        return !maze->isWall(nextX, nextY);
    }

protected:
    // Constants
    static constexpr float DEFAULT_SPEED = 130.0f;      // Pixels per second
    static constexpr float ANIMATION_INTERVAL = 0.1f;   // Seconds between animation frames

    // Reference to maze
    Maze* maze;

    // Grid position (in maze cells)
    int gridX;
    int gridY;

    // Position offset within grid cell (for smooth movement)
    float offsetX = 0.0f;
    float offsetY = 0.0f;

    // Movement properties
    Direction currentDirection = NONE;
    Direction nextDirection = NONE;
    float speed;

    // Game state
    bool active;

    // Animation properties
    bool animationState;
    float animationTimer;
    int animationFrame;

    // Define the character size
    float bodyRadius;

    // Visual representation
    bool texturesLoaded = false;

    // Find character's starting position - to be implemented by derived classes
    virtual void findStartPosition() = 0; // Pure virtual - must be implemented by derived classes

    // Update character's position
    virtual void updateSpritePosition() = 0; // Pure virtual - must be implemented by derived classes

    // Update animation state
    virtual void updateAnimation(float deltaTime) {
        // Base animation functionality
        animationTimer += deltaTime;

        // Toggle animation state
        if (animationTimer >= ANIMATION_INTERVAL) { 
            animationState = !animationState;
            animationTimer = 0.0f;
            animationFrame = (animationFrame + 1) % 4;  // Cycle through 4 animation frames
        }
    }

    // Update character's movement
    virtual void updateMovement(float deltaTime) {
        // Calculate movement speed for this frame
        float moveDistance = speed * deltaTime;

        // If we're perfectly aligned with the grid (no offset), check for direction changes
        if (offsetX == 0.0f && offsetY == 0.0f) {
            // Try to change direction if requested
            if (nextDirection != NONE && canMove(nextDirection)) {
                currentDirection = nextDirection;
            }

            // If we can't move in the current direction, stop
            if (currentDirection != NONE && !canMove(currentDirection)) {
                currentDirection = NONE;
            }
        }

        // If we have a direction, move in that direction
        if (currentDirection != NONE) {
            switch (currentDirection) {
            case RIGHT:
                offsetX += moveDistance;
                if (offsetX >= Maze::cellSize) {
                    gridX++;
                    offsetX = 0;
                    checkTunnel();
                }
                break;

            case LEFT:
                offsetX -= moveDistance;
                if (offsetX <= -Maze::cellSize) {
                    gridX--;
                    offsetX = 0;
                    checkTunnel();
                }
                break;

            case UP:
                offsetY -= moveDistance;
                if (offsetY <= -Maze::cellSize) {
                    gridY--;
                    offsetY = 0;
                }
                break;

            case DOWN:
                offsetY += moveDistance;
                if (offsetY >= Maze::cellSize) {
                    gridY++;
                    offsetY = 0;
                }
                break;

            default:
                break;
            }

            // Update the sprite position
            updateSpritePosition();
        }
    }

    // Check if character is in a tunnel and handle warping
    void checkTunnel() {
        // Left tunnel (character leaves left side)
        if (gridX < 0) {
            gridX = Maze::mazeW - 1;
        }
        // Right tunnel (character leaves right side)
        else if (gridX >= Maze::mazeW) {
            gridX = 0;
        }
    }
};

#endif // CHARACTER_H