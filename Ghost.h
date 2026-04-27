#ifndef GHOST_H
#define GHOST_H

#include "Character.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include "Pacman.h"

using namespace std;
using namespace sf;

// Forward declarations
class Pacman;
class Ghost;

// Define the GhostStrategy interface
class GhostStrategy {
public:
    virtual ~GhostStrategy() {}
    virtual void calculateNextMove(Ghost* ghost, Pacman* pacman, Maze* maze, int pacmanX, int pacmanY) = 0;
};

// Now define the Ghost class
class Ghost : public Character {
public:
    // Ghost types enum
    enum GhostType {
        RED = 0,
        BLUE = 1,
        PINK = 2,
        ORANGE = 3
    };

    // Ghost states enum
    enum GhostState {
        NORMAL = 0,
        FRIGHTENED = 1,
        EATEN = 2,
        COOLDOWN = 3
    };

    // Constructor
    Ghost(Maze* mazeRef, Pacman* pacmanRef, GhostType type) : Character(mazeRef) {
        this->pacmanRef = pacmanRef;
        ghostType = type;
        state = NORMAL;
        stateTimer = 0;
        speed = DEFAULT_SPEED*0.8;
        bodyRadius = Maze::cellSize / 2.0f - 2.0f;

        // Initialize flag values
        canLeaveInitialPosition = false;
        hasLeftGhostHouse = false;

        // Find starting position
        findStartPosition();

        // Store home position for returning later
        homeX = gridX;
        homeY = gridY;

        // Initialize path following variables
        hasPath = false;
        currentPathIndex = 0;

        // Set update interval for path recalculation (for moving targets)
        pathUpdateInterval = 30; // Update path every 30 frames
        framesSincePathUpdate = 0;

        // Load textures or setup fallback shapes
        if (!loadTextures()) {
            cout << "Failed to load ghost textures!" << endl;
            setupFallbackShapes();
        }

        // Set initial sprite and animation
        updateSpritePosition();
        animationFrame = 0;
        animationTimer = 0.0f;
        eyesDirection = RIGHT;

        // Initially no strategy
        movementStrategy = nullptr;

        cout << "Ghost created: " << getGhostName() << endl;
    }

    // Destructor
    ~Ghost() override {
        if (movementStrategy) {
            delete movementStrategy;
            movementStrategy = nullptr;
        }
    }

    // Check if ghost is in ghost house
    bool isInGhostHouse() const {
        return maze->isGhostHouse(gridX, gridY);
    }

    // Update method - called each frame
    void update(float deltaTime) override {
        // Update animation timer
        animationTimer += deltaTime;
        if (animationTimer >= ANIMATION_INTERVAL) {
            animationTimer = 0.0f;
            updateAppearance();
        }

        // Handle state timers
        if (stateTimer > 0) {
            stateTimer--;

            // When timer expires, transition to next state
            if (stateTimer <= 0) {
                if (state == COOLDOWN) {
                    // If cooldown is complete, return to normal
                    setState(NORMAL);
                    // Allow the ghost to leave again
                    canLeaveInitialPosition = true;
                    hasLeftGhostHouse = false;
                    cout << getGhostName() << " cooldown complete, can leave house again." << endl;
                }
                else if (state == FRIGHTENED) {
                    // If frightened wears off, return to normal
                    setState(NORMAL);
                }
            }
        }

        // Check if ghost has left the house
        if (canLeaveInitialPosition && !hasLeftGhostHouse && !isInGhostHouse()) {
            hasLeftGhostHouse = true;
            cout << getGhostName() << " has left the ghost house!" << endl;
        }

        // Determine movement based on current state
        if (state == EATEN) {
            // When eaten, go directly back to ghost house
            // If we don't have a path yet, calculate one
            if (!hasPath) {
                findPathToTarget(homeX, homeY, true); // true means stable target
            }

            // Follow the existing path
            followPath();

            // Check if we've reached home position
            if (gridX == homeX && gridY == homeY && offsetX == 0.0f && offsetY == 0.0f) {
                // Enter cooldown state
                setState(COOLDOWN, 180); // 3 seconds at 60 FPS
                hasPath = false; // Clear path since we've reached destination
                cout << getGhostName() << " reached home, entering cooldown." << endl;
            }
        }
        else if (state == COOLDOWN) {
            // During cooldown, stay in place
            currentDirection = NONE;
            nextDirection = NONE;
            hasPath = false; // Clear any path
        }
        else if (canLeaveInitialPosition) {
            if (!hasLeftGhostHouse) {
                // Focus on exiting the ghost house
                // If we don't have a path to exit, calculate one
                if (!hasPath) {
                    int exitX = 11; // Standard exit coordinates
                    int exitY = 10;
                    findPathToTarget(exitX, exitY, true); // true means stable target
                }

                // Follow the path to exit
                followPath();
            }
            else if (movementStrategy && pacmanRef) {
                // Once outside, follow appropriate strategy
                movementStrategy->calculateNextMove(this, pacmanRef, maze,
                    pacmanRef->getX(), pacmanRef->getY());
            }
        }

        // Update movement
        Character::updateMovement(deltaTime);

        // Update eyes direction
        updateEyesDirection();

        // Check if we've reached a waypoint in our path
        if (hasPath && currentPathIndex < path.size()) {
            // Get current waypoint
            pair<int, int> waypoint = path[currentPathIndex];

            // If we've reached this waypoint, move to next one
            if (gridX == waypoint.first && gridY == waypoint.second && offsetX == 0.0f && offsetY == 0.0f) {
                currentPathIndex++;

                // If we've reached the end of the path
                if (currentPathIndex >= path.size()) {
                    hasPath = false; // Clear path
                }
            }
        }
    }

    // Update eyes direction based on movement
    void updateEyesDirection() {
        if (currentDirection != NONE) {
            eyesDirection =currentDirection;
        }
    }

    // Get ghost type name
    string getGhostName() const {
        switch (ghostType) {
        case RED: return "Blinky (Red)";
        case BLUE: return "Inky (Blue)";
        case PINK: return "Pinky (Pink)";
        case ORANGE: return "Clyde (Orange)";
        default: return "Unknown Ghost";
        }
    }

    // Set ghost state
    void setState(GhostState newState, int duration = 0) {
        state = newState;
        stateTimer = duration;

        // Set speed based on state
        if (newState == EATEN) {
            speed = DEFAULT_SPEED * 5.0f; // Faster when eaten
        }
        else {
            speed = DEFAULT_SPEED*0.8;
        }

        // Clear path when state changes
        hasPath = false;
        path.clear();
        currentPathIndex = 0;

        updateAppearance();
    }

    // State check methods
    bool isFrightened() const { return state == FRIGHTENED; }
    bool isEaten() const { return state == EATEN; }
    bool isCooldown() const { return state == COOLDOWN; }

    // Set movement strategy
    void setStrategy(GhostStrategy* strategy) {
        if (movementStrategy) {
            delete movementStrategy;
        }
        movementStrategy = strategy;
    }

    // Called when eaten by Pacman
    void getEaten() {
        setState(EATEN);
        hasPath = false; // Force recalculation of path back to home
        cout << getGhostName() << " eaten! Returning to ghost house." << endl;
    }

    // Draw ghost to screen
    void draw(RenderWindow& window) override {
        if (texturesLoaded) {
            window.draw(sprite);
        }
        else {
            // Draw fallback shapes
            if (state != EATEN) {
                window.draw(ghostBody);
            }
            window.draw(leftEye);
            window.draw(rightEye);
            window.draw(leftPupil);
            window.draw(rightPupil);
        }

       /*  DEBUG: Draw path (uncomment to see the path visually)*/
        
        /*if (hasPath) {
            for (size_t i = currentPathIndex; i < path.size(); i++) {
                CircleShape dot;
                dot.setRadius(2.0f);
                dot.setFillColor(Color::Yellow);
                dot.setPosition(path[i].first * Maze::cellSize, path[i].second * Maze::cellSize);
                window.draw(dot);
            }
        }*/
        
    }

    // Get current direction
    Direction getDirection() const {
        return currentDirection;
    }

    // Find a path to target
    void findPathToTarget(int targetX, int targetY, bool isStableTarget) {
        // Clear existing path
        path.clear();
        currentPathIndex = 0;

        // Simplified path finding - just find any valid path

        // Start with starting position
        vector<pair<int, int>> queue;
        vector<vector<bool>> visited(Maze::mazeW, vector<bool>(Maze::mazeH, false));
        vector<vector<pair<int, int>>> parent(Maze::mazeW, vector<pair<int, int>>(Maze::mazeH, { -1, -1 }));

        // Start from current position
        queue.push_back({ gridX, gridY });
        visited[gridX][gridY] = true;

        bool foundPath = false;

        // Breadth-first search
        for (size_t i = 0; i < queue.size(); i++) {
            int x = queue[i].first;
            int y = queue[i].second;

            // Check if we've reached the target
            if (x == targetX && y == targetY) {
                foundPath = true;
                break;
            }

            // Try all four directions
            for (int d = 0; d < 4; d++) {
                int nx = x;
                int ny = y;

                // Calculate next position based on direction
                switch (d) {
                case 0: ny--; break; // Up
                case 1: nx++; break; // Right
                case 2: ny++; break; // Down
                case 3: nx--; break; // Left
                }

                // Handle tunnel wrapping
                if (nx < 0) nx = Maze::mazeW - 1;
                if (nx >= Maze::mazeW) nx = 0;

                // Skip if out of bounds vertically
                if (ny < 0 || ny >= Maze::mazeH) continue;

                // Skip if visited or is a wall
                if (visited[nx][ny] || maze->isWall(nx, ny)) continue;

                // Skip ghost house unless we're eaten
                if (maze->isGhostHouse(nx, ny) && state != EATEN && isInGhostHouse() == false) continue;

                // Add to queue
                queue.push_back({ nx, ny });
                visited[nx][ny] = true;
                parent[nx][ny] = { x, y };
            }
        }

        // Reconstruct path if found
        if (foundPath) {
            // Start from target
            int x = targetX;
            int y = targetY;

            // Temporary path (will be in reverse order)
            vector<pair<int, int>> tempPath;

            // Backtrack until we reach start
            while (!(x == gridX && y == gridY)) {
                tempPath.push_back({ x, y });
                pair<int, int> p = parent[x][y];
                x = p.first;
                y = p.second;
            }

            // Reverse path to get correct order
            for (int i = tempPath.size() - 1; i >= 0; i--) {
                path.push_back(tempPath[i]);
            }

            hasPath = true;
            currentPathIndex = 0;

            cout << getGhostName() << " found path with " << path.size() << " steps to ("
                << targetX << "," << targetY << ")" << endl;
        }
        else {
            // If no path found, we'll fall back to the traditional movement method
            cout << getGhostName() << " couldn't find path to (" << targetX << "," << targetY
                << "), falling back to direct movement" << endl;
            hasPath = false;
        }
    }

    // Follow the current path
    void followPath() {
        if (!hasPath || currentPathIndex >= path.size()) {
            return; // No path to follow
        }

        // Get the next waypoint
        pair<int, int> waypoint = path[currentPathIndex];

        // Calculate direction to waypoint
        moveDirectlyToTarget(waypoint.first, waypoint.second);
    }

    // Direct movement to a specific target (used when following path)
    void moveDirectlyToTarget(int targetX, int targetY) {
        // Calculate direction to go from current position to target
        Direction bestDirection = NONE;

        // If already at target, no need to move
        if (gridX == targetX && gridY == targetY) {
            return;
        }

        // Determine which direction to go
        if (gridX < targetX && canMove(RIGHT)) {
            bestDirection = RIGHT;
        }
        else if (gridX > targetX && canMove(LEFT)) {
            bestDirection = LEFT;
        }
        else if (gridY < targetY && canMove(DOWN)) {
            bestDirection = DOWN;
        }
        else if (gridY > targetY && canMove(UP)) {
            bestDirection = UP;
        }
        else {
            // If direct path is blocked, try other directions
            if (canMove(RIGHT)) bestDirection = RIGHT;
            else if (canMove(DOWN)) bestDirection = DOWN;
            else if (canMove(LEFT)) bestDirection = LEFT;
            else if (canMove(UP)) bestDirection = UP;
        }

        // Set the direction
        if (bestDirection != NONE) {
            setTargetDirection(bestDirection);
        }
    }

    // Move towards target position (for strategies to use)
    void move(int targetX, int targetY) {
        // For moving targets (like Pacman), we want to:
        // 1. Periodically recalculate the path
        // 2. In between recalculations, just follow the existing path

        framesSincePathUpdate++;

        // Calculate a new path at intervals or if we don't have a path
        if (!hasPath || framesSincePathUpdate >= pathUpdateInterval) {
            findPathToTarget(targetX, targetY, false); // false means moving target
            framesSincePathUpdate = 0;
        }

        // Follow the path if we have one
        if (hasPath) {
            followPath();
        }
        else {
            // Fallback to traditional movement if we have no path
            moveTraditional(targetX, targetY);
        }
    }

    // Original movement method (fallback)
    void moveTraditional(int targetX, int targetY) {
        // Calculate distance to target
        int deltaX = targetX - getX();
        int deltaY = targetY - getY();

        // Check available directions
        bool canMoveUp = canMove(UP);
        bool canMoveDown = canMove(DOWN);
        bool canMoveLeft = canMove(LEFT);
        bool canMoveRight = canMove(RIGHT);

        // Get current direction
        Direction currentDir = getDirection();

        // Don't reverse direction unless necessary
        Direction oppositeDir = getOppositeDirection(currentDir);

        // Determine preferred directions
        bool preferHorizontal = abs(deltaX) >= abs(deltaY);
        Direction firstChoice = NONE;
        Direction secondChoice = NONE;

        if (preferHorizontal) {
            // Target is more horizontal than vertical
            firstChoice = (deltaX > 0) ? RIGHT : LEFT;
            secondChoice = (deltaY > 0) ? DOWN : UP;
        }
        else {
            // Target is more vertical than horizontal
            firstChoice = (deltaY > 0) ? DOWN : UP;
            secondChoice = (deltaX > 0) ? RIGHT : LEFT;
        }

        // Try first choice
        if ((firstChoice == UP && canMoveUp) ||
            (firstChoice == DOWN && canMoveDown) ||
            (firstChoice == LEFT && canMoveLeft) ||
            (firstChoice == RIGHT && canMoveRight)) {
            setTargetDirection(firstChoice);
            return;
        }

        // Try second choice
        if ((secondChoice == UP && canMoveUp) ||
            (secondChoice == DOWN && canMoveDown) ||
            (secondChoice == LEFT && canMoveLeft) ||
            (secondChoice == RIGHT && canMoveRight)) {
            setTargetDirection(secondChoice);
            return;
        }

        // Try any direction except opposite
        if (canMoveUp && oppositeDir != UP) {
            setTargetDirection(UP);
        }
        else if (canMoveDown && oppositeDir != DOWN) {
            setTargetDirection(DOWN);
        }
        else if (canMoveLeft && oppositeDir != LEFT) {
            setTargetDirection(LEFT);
        }
        else if (canMoveRight && oppositeDir != RIGHT) {
            setTargetDirection(RIGHT);
        }
        else {
            // If all else fails, allow reversing
            if (canMoveUp) setTargetDirection(UP);
            else if (canMoveDown) setTargetDirection(DOWN);
            else if (canMoveLeft) setTargetDirection(LEFT);
            else if (canMoveRight) setTargetDirection(RIGHT);
        }
    }

    // Reset ghost to initial state
    void reset() override {
        // Call base reset
        Character::reset();

        // Reset ghost-specific state
        state = NORMAL;
        stateTimer = 0;
        speed = DEFAULT_SPEED*0.8;
        canLeaveInitialPosition = false;
        hasLeftGhostHouse = false;

        // Clear any path
        hasPath = false;
        path.clear();
        currentPathIndex = 0;
        framesSincePathUpdate = 0;

        // Reset appearance
        updateAppearance();
    }

    // Set target position
    void setTarget(int newTargetX, int newTargetY) {
        targetX = newTargetX;
        targetY = newTargetY;
    }

    // Check for collision with Pacman
    bool checkCollision(int pacmanX, int pacmanY) {
        // No collision if still in house, in cooldown, or can't leave
        if (!canLeaveInitialPosition || !hasLeftGhostHouse || state == COOLDOWN) {
            return false;
        }

        // Check if in same cell
        if (gridX == pacmanX && gridY == pacmanY) {
            return true;
        }

        // Check for collision during transition
        float ghostScreenX = gridX * Maze::cellSize + offsetX;
        float ghostScreenY = gridY * Maze::cellSize + offsetY;
        float pacmanScreenX = pacmanX * Maze::cellSize + pacmanRef->getOffsetX();
        float pacmanScreenY = pacmanY * Maze::cellSize + pacmanRef->getOffsetY();

        float distance = sqrt(pow(ghostScreenX - pacmanScreenX, 2) +
            pow(ghostScreenY - pacmanScreenY, 2));

        return distance < (bodyRadius * 0.8f);
    }

    // Pacman reference methods
    void setPacmanRef(Pacman* ref) { pacmanRef = ref; }
    Pacman* getPacmanRef() const { return pacmanRef; }

    // Set flag to allow leaving ghost house
    void setCanLeave(bool canLeave) {
        bool wasAllowed = canLeaveInitialPosition;
        canLeaveInitialPosition = canLeave;

        if (canLeave && !wasAllowed) {
            // Reset movement direction
            currentDirection = NONE;
            nextDirection = NONE;

            // Reset house exit flag
            hasLeftGhostHouse = false;

            // Clear any path
            hasPath = false;
            path.clear();

            cout << getGhostName() << " is now allowed to leave at position ("
                << gridX << "," << gridY << ")" << endl;
        }
    }

    // Check if ghost can leave
    bool canLeave() const {
        return canLeaveInitialPosition;
    }

    // Target management
    void setTargetDirection(Direction dir) {
        nextDirection = dir;
    }

    Direction getTargetDirection() const {
        return nextDirection;
    }

    int getTargetX() const {
        return targetX;
    }

    int getTargetY() const {
        return targetY;
    }

    // Override canMove to handle ghost house logic
    bool canMove(Direction dir) override {
        // Calculate target cell
        int nextX = gridX;
        int nextY = gridY;

        switch (dir) {
        case RIGHT: nextX++; break;
        case LEFT: nextX--; break;
        case UP: nextY--; break;
        case DOWN: nextY++; break;
        default: return false; // NONE direction can't move
        }

        // Handle tunnel wrapping
        if (nextX < 0) nextX = Maze::mazeW - 1;
        if (nextX >= Maze::mazeW) nextX = 0;

        // Check for walls
        if (maze->isWall(nextX, nextY)) {
            return false;
        }

        // Check ghost house status
        bool currentlyInGhostHouse = isInGhostHouse();
        bool targetIsGhostHouse = maze->isGhostHouse(nextX, nextY);

        // If in cooldown, don't allow movement
        if (state == COOLDOWN) {
            return false;
        }

        // If eaten, allow entering ghost house
        if (state == EATEN) {
            return !maze->isWall(nextX, nextY);
        }

        // If trying to exit ghost house
        if (canLeaveInitialPosition && !hasLeftGhostHouse && currentlyInGhostHouse) {
            return !maze->isWall(nextX, nextY);
        }

        // If not allowed to leave yet and trying to exit
        if (!canLeaveInitialPosition && currentlyInGhostHouse && !targetIsGhostHouse) {
            return false;
        }

        // Prevent normal ghosts from entering ghost house
        if (targetIsGhostHouse && !currentlyInGhostHouse && state != EATEN) {
            return false;
        }

        return true;
    }

    // Method specifically for exiting ghost house
    void moveTowardsExit() {
        // The exit is typically around coordinates (11,10)
        int exitX = 11;
        int exitY = 10;

        // If we don't have a path to the exit, calculate one
        if (!hasPath) {
            findPathToTarget(exitX, exitY, true); // true means stable target
        }

        // Follow the path
        followPath();
    }

    // Method to move directly back to home position (when eaten)
    void moveTowardsHome() {
        // If we don't have a path home, calculate one
        if (!hasPath) {
            findPathToTarget(homeX, homeY, true); // true means stable target
        }

        // Follow the path
        followPath();
    }

    // Choose random valid direction
    void chooseRandomValidDirection() {
        vector<Direction> validDirections;

        if (canMove(UP)) validDirections.push_back(UP);
        if (canMove(DOWN)) validDirections.push_back(DOWN);
        if (canMove(LEFT)) validDirections.push_back(LEFT);
        if (canMove(RIGHT)) validDirections.push_back(RIGHT);

        if (!validDirections.empty()) {
            int randomIndex = rand() % validDirections.size();
            setTargetDirection(validDirections[randomIndex]);
        }
    }

    // Get opposite direction
    Direction getOppositeDirection(Direction dir) const {
        switch (dir) {
        case UP: return DOWN;
        case DOWN: return UP;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
        default: return NONE;
        }
    }

    // Flag for tracking if ghost has left the house
    bool hasLeftGhostHouse;

protected:
    bool loadTextures() {
        bool success = false;

        // Load normal texture based on ghost type
        switch (ghostType) {
        case RED: success = normalTexture.loadFromFile("texture/blinky.png"); break;
        case BLUE: success = normalTexture.loadFromFile("texture/inky.png"); break;
        case PINK: success = normalTexture.loadFromFile("texture/pinky.png"); break;
        case ORANGE: success = normalTexture.loadFromFile("texture/clyde.png"); break;
        }

        if (!success) {
            return false;
        }

        // Load the frightened texture
        frightenedTexture.loadFromFile("texture/blue_ghost.png");

        // For eaten state, we'll use the eyes texture or make normal semi-transparent
        eatenTexture = normalTexture;

        // Set up the sprite
        sprite.setTexture(normalTexture, true);

        // Set size and origin
        float desiredWidth = Maze::cellSize * 0.8f;
        float scaleX = desiredWidth / sprite.getLocalBounds().width;
        float scaleY = scaleX; // Keep aspect ratio
        sprite.setScale(scaleX, scaleY);
        sprite.setOrigin(sprite.getLocalBounds().width / 2.0f,
            sprite.getLocalBounds().height / 2.0f);

        texturesLoaded = true;
        return true;
    }

    // Set up fallback shapes
    void setupFallbackShapes() {
        // Create ghost body
        ghostBody.setRadius(bodyRadius);

        // Set color based on ghost type
        switch (ghostType) {
        case RED: ghostBody.setFillColor(Color::Red); break;
        case BLUE: ghostBody.setFillColor(Color::Blue); break;
        case PINK: ghostBody.setFillColor(Color::Magenta); break;
        case ORANGE: ghostBody.setFillColor(Color(255, 165, 0)); break;
        }

        ghostBody.setOrigin(bodyRadius, bodyRadius);

        // Create eyes
        float eyeRadius = bodyRadius / 4.0f;
        leftEye.setRadius(eyeRadius);
        rightEye.setRadius(eyeRadius);
        leftEye.setFillColor(Color::White);
        rightEye.setFillColor(Color::White);
        leftEye.setOrigin(eyeRadius, eyeRadius);
        rightEye.setOrigin(eyeRadius, eyeRadius);

        // Create pupils
        float pupilRadius = eyeRadius / 2.0f;
        leftPupil.setRadius(pupilRadius);
        rightPupil.setRadius(pupilRadius);
        leftPupil.setFillColor(Color::Blue);
        rightPupil.setFillColor(Color::Blue);
        leftPupil.setOrigin(pupilRadius, pupilRadius);
        rightPupil.setOrigin(pupilRadius, pupilRadius);
    }

    // Update appearance based on state
    void updateAppearance() {
        if (texturesLoaded) {
            switch (state) {
            case NORMAL:
            case COOLDOWN:
                sprite.setTexture(normalTexture);
                sprite.setColor(Color(255, 255, 255, 255)); // Fully opaque
                break;
            case FRIGHTENED:
                sprite.setTexture(frightenedTexture);
                sprite.setColor(Color(255, 255, 255, 255)); // Fully opaque
                break;
            case EATEN:
                sprite.setTexture(eatenTexture);
                sprite.setColor(Color(255, 255, 255, 128)); // Semi-transparent
                break;
            }
        }
        else {
            // Update fallback shapes
            if (state == FRIGHTENED) {
                ghostBody.setFillColor(Color::Blue);
            }
            else if (state == EATEN) {
                ghostBody.setFillColor(Color::Transparent); // Show only eyes
            }
            else {
                // Reset to normal color
                switch (ghostType) {
                case RED: ghostBody.setFillColor(Color::Red); break;
                case BLUE: ghostBody.setFillColor(Color::Blue); break;
                case PINK: ghostBody.setFillColor(Color::Magenta); break;
                case ORANGE: ghostBody.setFillColor(Color(255, 165, 0)); break;
                }
            }
        }
    }

    // Find starting position based on ghost type
    void findStartPosition() override {
        char ghostMarker;
        switch (ghostType) {
        case RED: ghostMarker = 'B'; break;    // Blinky
        case BLUE: ghostMarker = 'I'; break;   // Inky
        case PINK: ghostMarker = 'P'; break;   // Pinky
        case ORANGE: ghostMarker = 'C'; break; // Clyde
        default: ghostMarker = '*'; break;     // Default ghost house position
        }

        bool found = false;

        // Look for specific ghost marker
        for (int y = 0; y < Maze::mazeH; y++) {
            for (int x = 0; x < Maze::mazeW; x++) {
                string cell = maze->getCellAt(x, y);
                if (!cell.empty() && cell[0] == ghostMarker) {
                    gridX = x;
                    gridY = y;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        // If specific marker not found, look for ghost house '*'
        if (!found) {
            for (int y = 0; y < Maze::mazeH; y++) {
                for (int x = 0; x < Maze::mazeW; x++) {
                    if (maze->getCellAt(x, y) == "*") {
                        gridX = x;
                        gridY = y;
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
        }

        // Default position if no markers found
        if (!found) {
            gridX = 11;
            gridY = 11; // Center of ghost house
        }
    }

    // Update sprite position
    void updateSpritePosition() override {
        Vector2f screenPos = maze->gridToScreen(gridX, gridY);
        screenPos.x += offsetX;
        screenPos.y += offsetY;

        if (texturesLoaded) {
            sprite.setPosition(screenPos);
        }
        else {
            // Fallback shape positioning
            ghostBody.setPosition(screenPos);

            // Position eyes
            float eyeOffsetX = bodyRadius / 2.5f;
            float eyeOffsetY = -bodyRadius / 3.0f;
            leftEye.setPosition(screenPos.x - eyeOffsetX, screenPos.y + eyeOffsetY);
            rightEye.setPosition(screenPos.x + eyeOffsetX, screenPos.y + eyeOffsetY);

            // Position pupils based on direction
            float pupilOffsetX = 0.0f;
            float pupilOffsetY = 0.0f;

            switch (eyesDirection) {
            case RIGHT: pupilOffsetX = bodyRadius / 10.0f; break;
            case LEFT: pupilOffsetX = -bodyRadius / 10.0f; break;
            case UP: pupilOffsetY = -bodyRadius / 10.0f; break;
            case DOWN: pupilOffsetY = bodyRadius / 10.0f; break;
            }

            leftPupil.setPosition(leftEye.getPosition().x + pupilOffsetX,
                leftEye.getPosition().y + pupilOffsetY);
            rightPupil.setPosition(rightEye.getPosition().x + pupilOffsetX,
                rightEye.getPosition().y + pupilOffsetY);
        }
    }

private:
    // Ghost type and state
    GhostType ghostType;
    GhostState state;
    int stateTimer;
    int homeX, homeY;   // Home position coordinates
    int targetX, targetY;
    Direction eyesDirection;
    GhostStrategy* movementStrategy;
    Pacman* pacmanRef;
    bool canLeaveInitialPosition;

    // Path following variables
    bool hasPath;
    vector<pair<int, int>> path;
    size_t currentPathIndex;
    int pathUpdateInterval;
    int framesSincePathUpdate;

    // Visual elements
    Texture normalTexture;
    Texture frightenedTexture;
    Texture eatenTexture;
    Sprite sprite;

    // Fallback shapes
    CircleShape ghostBody;
    CircleShape leftEye;
    CircleShape rightEye;
    CircleShape leftPupil;
    CircleShape rightPupil;
};

// Random movement strategy
// Random movement strategy
class RandomStrategy : public GhostStrategy {
public:
    void calculateNextMove(Ghost* ghost, Pacman* pacman, Maze* maze, int pacmanX, int pacmanY) override {
        // Skip if in ghost house and hasn't left yet
        if (ghost->isInGhostHouse() && !ghost->hasLeftGhostHouse) {
            return;
        }

        // Calculate distance to Pacman
        int distanceToPacman = calculateDistance(ghost->getX(), ghost->getY(), pacmanX, pacmanY);

        // If ghost is too far from Pacman (more than 1/3 of the board), directly pursue him
        int maxDistance = (Maze::mazeW + Maze::mazeH) / 4; // One-third of average board dimension

        if (distanceToPacman > maxDistance) {
            // Temporarily use direct pursuit to get closer to Pacman
            ghost->setTarget(pacmanX, pacmanY);
            ghost->move(ghost->getTargetX(), ghost->getTargetY());
            return; // Skip random movement logic
        }

        // Normal random movement logic (used when close enough to Pacman)
        Character::Direction currentDir = ghost->getDirection();

        if (currentDir == Character::NONE ||
            !ghost->canMove(currentDir) ||
            isAtJunction(ghost, maze)) {

            // Choose new random direction (avoiding opposite if possible)
            std::vector<Character::Direction> possibleDirs;
            Character::Direction oppositeDir = getOppositeDirection(currentDir);

            // Collect valid directions (non-opposite preferred)
            if (ghost->canMove(Character::UP) && Character::UP != oppositeDir)
                possibleDirs.push_back(Character::UP);
            if (ghost->canMove(Character::DOWN) && Character::DOWN != oppositeDir)
                possibleDirs.push_back(Character::DOWN);
            if (ghost->canMove(Character::LEFT) && Character::LEFT != oppositeDir)
                possibleDirs.push_back(Character::LEFT);
            if (ghost->canMove(Character::RIGHT) && Character::RIGHT != oppositeDir)
                possibleDirs.push_back(Character::RIGHT);

            // If no non-opposite directions, allow any valid direction
            if (possibleDirs.empty()) {
                if (ghost->canMove(Character::UP))
                    possibleDirs.push_back(Character::UP);
                if (ghost->canMove(Character::DOWN))
                    possibleDirs.push_back(Character::DOWN);
                if (ghost->canMove(Character::LEFT))
                    possibleDirs.push_back(Character::LEFT);
                if (ghost->canMove(Character::RIGHT))
                    possibleDirs.push_back(Character::RIGHT);
            }

            // Pick random direction from options
            if (!possibleDirs.empty()) {
                int randomIndex = rand() % possibleDirs.size();
                ghost->setTargetDirection(possibleDirs[randomIndex]);
            }
        }
    }

private:
    // Calculate Manhattan distance between two grid positions
    int calculateDistance(int x1, int y1, int x2, int y2) {
        return abs(x1 - x2) + abs(y1 - y2);
    }

    bool isAtJunction(Ghost* ghost, Maze* maze) {
        int options = 0;
        if (ghost->canMove(Character::UP)) options++;
        if (ghost->canMove(Character::DOWN)) options++;
        if (ghost->canMove(Character::LEFT)) options++;
        if (ghost->canMove(Character::RIGHT)) options++;
        return options > 2; // More than 2 directions = junction
    }

    Character::Direction getOppositeDirection(Character::Direction dir) {
        switch (dir) {
        case Character::UP: return Character::DOWN;
        case Character::DOWN: return Character::UP;
        case Character::LEFT: return Character::RIGHT;
        case Character::RIGHT: return Character::LEFT;
        default: return Character::NONE;
        }
    }
};


// Direct pursuit strategy (Blinky)
class DirectPursuitStrategy : public GhostStrategy {
public:
    void calculateNextMove(Ghost* ghost, Pacman* pacman, Maze* maze, int pacmanX, int pacmanY) override {
        // If frightened, use random movement
        if (ghost->isFrightened()) {
            RandomStrategy randomMove;
            randomMove.calculateNextMove(ghost, pacman, maze, pacmanX, pacmanY);
            return;
        }

        // Skip if in ghost house and hasn't left yet
        if (ghost->isInGhostHouse() && !ghost->hasLeftGhostHouse) {
            return;
        }

        // Direct pursuit - chase Pacman directly
        ghost->setTarget(pacmanX, pacmanY);
        ghost->move(ghost->getTargetX(), ghost->getTargetY());
    }
};

// Ambush strategy (Pinky)
class AmbushStrategy : public GhostStrategy {
public:
    void calculateNextMove(Ghost* ghost, Pacman* pacman, Maze* maze, int pacmanX, int pacmanY) override {
        // If frightened, use random movement
        if (ghost->isFrightened()) {
            RandomStrategy randomMove;
            randomMove.calculateNextMove(ghost, pacman, maze, pacmanX, pacmanY);
            return;
        }

        // Skip if in ghost house and hasn't left yet
        if (ghost->isInGhostHouse() && !ghost->hasLeftGhostHouse) {
            return;
        }

        // Get Pacman's direction
        Character::Direction pacDir = pacman->getDirection();

        // Default to current position
        int targetX = pacmanX;
        int targetY = pacmanY;

        // Try to predict 4 tiles ahead
        const int predictSteps = 4;

        switch (pacDir) {
        case Character::RIGHT:
            targetX = min(pacmanX + predictSteps, Maze::mazeW - 1);
            // Check for walls
            for (int x = pacmanX + 1; x <= targetX; x++) {
                if (maze->isWall(x, pacmanY)) {
                    targetX = x - 1; // Stop before wall
                    break;
                }
            }
            break;

        case Character::LEFT:
            targetX = std::max(pacmanX - predictSteps, 0);
            // Check for walls
            for (int x = pacmanX - 1; x >= targetX; x--) {
                if (maze->isWall(x, pacmanY)) {
                    targetX = x + 1; // Stop before wall
                    break;
                }
            }
            break;

        case Character::UP:
            targetY = std::max(pacmanY - predictSteps, 0);
            // Check for walls
            for (int y = pacmanY - 1; y >= targetY; y--) {
                if (maze->isWall(pacmanX, y)) {
                    targetY = y + 1; // Stop before wall
                    break;
                }
            }
            break;

        case Character::DOWN:
            targetY = std::min(pacmanY + predictSteps, Maze::mazeH - 1);
            // Check for walls
            for (int y = pacmanY + 1; y <= targetY; y++) {
                if (maze->isWall(pacmanX, y)) {
                    targetY = y - 1; // Stop before wall
                    break;
                }
            }
            break;

        case Character::NONE:
            // If Pacman isn't moving, just target his position
            break;
        }

        // Move toward the target
        ghost->setTarget(targetX, targetY);
        ghost->move(ghost->getTargetX(), ghost->getTargetY());
    }
};

class PatrolStrategy : public GhostStrategy {
public:
    PatrolStrategy() : hasTarget(false), targetX(0), targetY(0) {}

    void calculateNextMove(Ghost* ghost, Pacman* pacman, Maze* maze, int pacmanX, int pacmanY) override {
        // If frightened, use random movement
        if (ghost->isFrightened()) {
            RandomStrategy randomMove;
            randomMove.calculateNextMove(ghost, pacman, maze, pacmanX, pacmanY);
            return;
        }

        // Skip if in ghost house and hasn't left yet
        if (ghost->isInGhostHouse() && !ghost->hasLeftGhostHouse) {
            return;
        }

        // Calculate distance to Pacman
        int distanceToPacman = calculateDistance(ghost->getX(), ghost->getY(), pacmanX, pacmanY);

        // If ghost is too far from Pacman (more than 1/3 of the board), directly pursue him
        int maxDistance = (Maze::mazeW + Maze::mazeH) / 4; 

        if (distanceToPacman > maxDistance) {
            // Temporarily use direct pursuit to get closer to Pacman
            ghost->setTarget(pacmanX, pacmanY);
            ghost->move(pacmanX, pacmanY);
            // Reset target so we'll generate a new one when we're close enough
            hasTarget = false;
            return; // Skip patrol movement logic
        }

        // Normal patrol logic (used when close enough to Pacman)
        // Check if we need a new target
        if (!hasTarget) {
            // Generate new patrol target
            generateNewTarget(ghost, maze);

            // Find path to the target
            ghost->findPathToTarget(targetX, targetY, true); // true = stable target

            hasTarget = true;
            cout << "Patrol ghost set new target: (" << targetX << "," << targetY << ")" << endl;
        }

        // Check if we've reached the target
        if (ghost->getX() == targetX && ghost->getY() == targetY) {
            // Target reached, get a new one
            hasTarget = false;
        }
        else {
            // Move towards the target
            ghost->setTarget(targetX, targetY);
            ghost->move(targetX, targetY);
        }
    }

private:
    bool hasTarget;
    int targetX, targetY;

    // Calculate Manhattan distance between two grid positions
    int calculateDistance(int x1, int y1, int x2, int y2) {
        return abs(x1 - x2) + abs(y1 - y2);
    }

    // Generate a new patrol target
    void generateNewTarget(Ghost* ghost, Maze* maze) {
        for (int attempts = 0; attempts < 20; attempts++) {
            // Pick random position
            int range = 8;
            int newX = ghost->getX() + (rand() % (2 * range)) - range;
            int newY = ghost->getY() + (rand() % (2 * range)) - range;

            // Ensure in bounds
            newX = max(0, min(newX, Maze::mazeW - 1));
            newY = max(0, min(newY, Maze::mazeH - 1));

            // Check if valid
            if (!maze->isWall(newX, newY) && !maze->isGhostHouse(newX, newY)) {
                targetX = newX;
                targetY = newY;
                return;
            }
        }

        // Fallback - find any valid position
        for (int y = 0; y < Maze::mazeH; y++) {
            for (int x = 0; x < Maze::mazeW; x++) {
                if (!maze->isWall(x, y) && !maze->isGhostHouse(x, y)) {
                    targetX = x;
                    targetY = y;
                    return;
                }
            }
        }
    }
};

#endif // GHOST_H