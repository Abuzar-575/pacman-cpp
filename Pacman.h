#ifndef PACMAN_H
#define PACMAN_H

#include "Character.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <array>

using namespace std;
using namespace sf;

class Pacman : public Character {
public:
   
    static constexpr float DEFAULT_SPEED = 130.0f;      // Pixels per second
    static constexpr float ANIMATION_INTERVAL = 0.1f;

    // Constructor
    Pacman(Maze* mazeRef) : Character(mazeRef) {
        cout << "Pacman class OK" << endl;

        // Pacman-specific initialization
        mouthOpen = true;
        animationTimer = 0.0f;
        animationFrame = 0;
        lives = 3;
        score = 0;
        powerMode = false;
        powerTimer = 0;
        consecutiveGhostsEaten = 0;

        // Find the starting position
        findStartPosition();

        // Initialize texture and sprite
        if (!loadTextures()) {
            cout << "Failed to load Pacman textures!" << endl;
            setupFallbackShapes();
        }

        // Set initial sprite position
        updateSpritePosition();
    }

    // Destructor
    ~Pacman() override {
        // Cleanup if needed
    }
    bool isPowerMode() const {
        return powerMode;
    }
    int getLives() const {
        return lives;
    }
    // Called when Pacman is caught by a ghost
    void loseLife() {
        lives--;
        respawn();
        powerMode = false;
        powerTimer = 0;
        consecutiveGhostsEaten = 0;
        // Reset speed - ADD THIS LINE
        speed = DEFAULT_SPEED;
    }

    // Called when Pacman eats a ghost
    void eatGhost() {
        consecutiveGhostsEaten++;
        // Points increase for each consecutive ghost eaten
        int points = 200 * (1 << (consecutiveGhostsEaten - 1)); // 200, 400, 800, 1600
        score += points;
    }
	float getOffsetX() {
		// Return the offsetX value
		return offsetX;
	}
    float getOffsetY() {
        // Return the offsetY value
        return offsetY;
    }

    // Get the point value for the next ghost to be eaten
    int getNextGhostValue() const {
        if (!powerMode) return 0;
        return 200 * (1 << consecutiveGhostsEaten); // Points for next ghost
    }

    // Override update from Character
    void update(float deltaTime) override {
        // Use base class animation and movement updates
        Character::update(deltaTime);

        // Pacman-specific update: check if we consumed a dot
        checkDotConsumption();

        // Update power mode timer if active
        if (powerMode && powerTimer > 0) {
            powerTimer--;
            if (powerTimer <= 0) {
                powerMode = false;
                consecutiveGhostsEaten = 0;
                // Reset speed when power mode ends - ADD THIS LINE
                speed = DEFAULT_SPEED;
            }
        }
    }

    // Override draw from Character
    void draw(RenderWindow& window) override {
        if (texturesLoaded) {
            window.draw(sprite);
        }
        else {
            // Fallback to basic shape drawing if textures failed
            window.draw(pacmanBody);

            // Only draw the mouth when it's open
            if (mouthOpen) {
                window.draw(pacmanMouth);
            }

            window.draw(pacmanEye);
        }
    }
    Direction getDirection() {
        return currentDirection;
    }
    Pacman* getInstance() {
        // Return the instance of Pacman
        return this;
    }
    // Handle keyboard input
    void handleInput() {
        // Check keyboard for direction changes
        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            nextDirection = RIGHT;
        }
        else if (Keyboard::isKeyPressed(Keyboard::Left)) {
            nextDirection = LEFT;
        }
        else if (Keyboard::isKeyPressed(Keyboard::Up)) {
            nextDirection = UP;
        }
        else if (Keyboard::isKeyPressed(Keyboard::Down)) {
            nextDirection = DOWN;
        }
    }

    // Get current score
    int getScore() const {
        return score;
    }

    // Override reset from Character
    void reset() override {
        Character::reset(); // Call base class reset first
        score = 0;
        lives = 3;
        powerMode = false;
        powerTimer = 0;
        consecutiveGhostsEaten = 0;
    }

private:
    // Pacman-specific properties
    bool mouthOpen;
    bool powerMode = false;
    int powerTimer = 0;
    int lives = 3;                    // Number of lives remaining
    int score = 0;                    // Current score
            // Whether Pacman can eat ghosts (energizer effect)
                   // Timer for how long powerMode lasts
    int consecutiveGhostsEaten = 0;   // Number of ghosts eaten during current power mode

    // Visual representation
    array<Texture, 5> textures; // One texture for each direction
    Sprite sprite;

    // Fallback shapes when textures aren't available
    CircleShape pacmanBody;  // Yellow circle for body
    ConvexShape pacmanMouth; // Triangle for mouth
    CircleShape pacmanEye;   // Small circle for eye

    // Override findStartPosition from Character
    void findStartPosition() override {
        bool found = false;
        // Look for 'c' character in maze pattern
        for (int y = 0; y < Maze::mazeH; y++) {
            for (int x = 0; x < Maze::mazeW; x++) {
                if (maze->getCellAt(x, y) == "c") {
                    gridX = x;
                    gridY = y;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }

        // Fallback if 'c' not found
        if (!found) {
            gridX = 11;
            gridY = 18;
        }
    }

    // Load textures for pacman
    bool loadTextures() {
        // Try to load textures from the images provided
        if (!textures[0].loadFromFile("texture/PacMan.png") ||
            !textures[RIGHT].loadFromFile("texture/right.png") ||
            !textures[LEFT].loadFromFile("texture/left.png") ||
            !textures[UP].loadFromFile("texture/up.png") ||
            !textures[DOWN].loadFromFile("texture/down.png")) {
            return false;
        }

        // Set up initial sprite with first texture
        sprite.setTexture(textures[RIGHT]);

        // Center the sprite origin
        sprite.setOrigin(sprite.getLocalBounds().width / 2.0f,
            sprite.getLocalBounds().height / 2.0f);

        texturesLoaded = true;
        return true;
    }

    // Set up fallback shapes for when textures aren't available
    void setupFallbackShapes() {
        // Set up the body (yellow circle)
        pacmanBody.setRadius(bodyRadius);
        pacmanBody.setFillColor(Color::Yellow);
        pacmanBody.setOrigin(bodyRadius, bodyRadius);

        // Set up the mouth (triangular cut-out)
        pacmanMouth.setPointCount(3);
        pacmanMouth.setPoint(0, Vector2f(0, 0));
        pacmanMouth.setPoint(1, Vector2f(bodyRadius, bodyRadius / 2));
        pacmanMouth.setPoint(2, Vector2f(bodyRadius, -bodyRadius / 2));
        pacmanMouth.setFillColor(Color::Black);
        pacmanMouth.setOrigin(0, 0);

        // Set up the eye (small black circle)
        pacmanEye.setRadius(bodyRadius / 6);
        pacmanEye.setFillColor(Color::Black);
        pacmanEye.setOrigin(bodyRadius / 6, bodyRadius / 6);
        pacmanEye.setPosition(bodyRadius / 2, -bodyRadius / 2);
    }

    // Override updateSpritePosition from Character
    void updateSpritePosition() override {
        Vector2f screenPos = maze->gridToScreen(gridX, gridY);
        screenPos.x += offsetX;
        screenPos.y += offsetY;

        if (texturesLoaded) {
            sprite.setPosition(screenPos);
        }
        else {
            pacmanBody.setPosition(screenPos);

            // Update the mouth and eye based on direction
            float mouthAngle = 0.0f;
            switch (currentDirection) {
            case RIGHT: mouthAngle = 0.0f; break;
            case LEFT:  mouthAngle = 180.0f; break;
            case UP:    mouthAngle = 270.0f; break;
            case DOWN:  mouthAngle = 90.0f; break;
            default:    mouthAngle = 0.0f; break;
            }

            // Set mouth position and angle
            pacmanMouth.setRotation(mouthAngle);
            pacmanMouth.setPosition(screenPos);

            // Set eye position based on direction
            float eyeX = 0, eyeY = 0;
            switch (currentDirection) {
            case RIGHT: eyeX = bodyRadius / 2; eyeY = -bodyRadius / 2; break;
            case LEFT:  eyeX = -bodyRadius / 2; eyeY = -bodyRadius / 2; break;
            case UP:    eyeX = 0; eyeY = -bodyRadius / 2; break;
            case DOWN:  eyeX = 0; eyeY = bodyRadius / 2; break;
            default:    eyeX = bodyRadius / 2; eyeY = -bodyRadius / 2; break;
            }
            pacmanEye.setPosition(screenPos.x + eyeX, screenPos.y + eyeY);
        }
    }

    // Override updateAnimation with Pacman-specific animation
    void updateAnimation(float deltaTime) override {
        // Update animation timer
        animationTimer += deltaTime;

        // Toggle mouth state
        if (animationTimer >= ANIMATION_INTERVAL) {
            mouthOpen = !mouthOpen;
            animationTimer = 0.0f;
       

            // Update sprite texture based on direction and mouth state
            if (texturesLoaded) {
                // Select the right texture based on direction
                if (currentDirection != NONE) {
                    if (mouthOpen) {
                        sprite.setTexture(textures[currentDirection]); // Direction-specific with open mouth
                    }
                    else {
                        sprite.setTexture(textures[0]); // Closed mouth texture
                    }
                }
            }
        }
    }

    // Check for dot consumption at current position
    void checkDotConsumption() {
        // Only check for dot consumption when perfectly aligned with grid
        if (offsetX == 0.0f && offsetY == 0.0f && maze->hasDot(gridX, gridY)) {
            // Check if it's a power pellet (special handling)
            string cellContent = maze->getCellAt(gridX, gridY);
            if (cellContent == "@") {
                // Power pellet was eaten, activate power mode
                powerMode = true;
                powerTimer = 300; // 5 seconds at 60 FPS
                consecutiveGhostsEaten = 0;
                // Increase speed during power mode - ADD THIS LINE
                speed = DEFAULT_SPEED * 1.5f; // 50% speed increase
            }

            // Remove the dot and update score
            maze->removeDot(gridX, gridY);

            // Score is updated in the maze's removeDot method
            score = maze->score;
        }
    }

    // Reset position after losing a life
    void respawn() {
        // Reset position to starting position
        findStartPosition();

        // Reset movement
        currentDirection = NONE;
        nextDirection = NONE;
        offsetX = 0.0f;
        offsetY = 0.0f;

        // Update visual position
        updateSpritePosition();

        // Mark as active again
        active = true;
    }
    bool canMove(Direction dir) override {
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

        // First check the base class condition (walls)
        if (!Character::canMove(dir)) {
            return false;
        }

        // Then check if the target cell is part of the ghost house
        if (maze->isGhostHouse(nextX, nextY)) {
            return false;
        }

        return true;
    }
};

#endif // PACMAN_H  