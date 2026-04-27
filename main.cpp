#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include "Maze.h"
#include "Character.h"
#include "Pacman.h"
#include "Ghost.h"
#include "GameHUD.h" // Added include for the HUD
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>

using namespace std;
using namespace sf;

int main()
{
    // Initialize random seed
    srand(static_cast<unsigned int>(time(nullptr)));

    // Create the window
    RenderWindow window(VideoMode({ 700, 770 }), "PacMan");

    // Set frame rate limit
    window.setFramerateLimit(60);

    // Create the maze
    Maze maze;

    // Create Pacman
    Pacman pacman(&maze);

    // Create GameHUD for score and lives display
    GameHUD hud(&maze, &pacman);

    // Create Ghosts with different strategies
    Ghost redGhost(&maze, &pacman, Ghost::RED);
    Ghost pinkGhost(&maze, &pacman, Ghost::PINK);
    Ghost blueGhost(&maze, &pacman, Ghost::BLUE);
    Ghost orangeGhost(&maze, &pacman, Ghost::ORANGE);

    // Assign strategies
    redGhost.setStrategy(new DirectPursuitStrategy());    // Blinky - Direct chase
    pinkGhost.setStrategy(new AmbushStrategy());          // Pinky - Ambush ahead
    blueGhost.setStrategy(new PatrolStrategy());          // Inky - Patrol/chase combo
    orangeGhost.setStrategy(new RandomStrategy());        // Clyde - Random movement

    // Store all ghosts in a vector for easy updating and drawing
    vector<Ghost*> ghosts = { &redGhost, &pinkGhost, &blueGhost, &orangeGhost };

    // Initialize ghosts - ensure they are all set to not leave initially
    cout << "Initializing ghosts..." << endl;
    for (size_t i = 0; i < ghosts.size(); i++) {
        ghosts[i]->reset();  // Makes sure positions are reset
        ghosts[i]->setCanLeave(false);
    }

    // Clock for delta time
    Clock clock;

    // Game timer for ghost release
    int gameTimer = 0;

    // Flag to track if game is active
    bool gameActive = true;

    bool wasPowerMode = false;

    // Main game loop
    while (window.isOpen())
    {
        // Calculate delta time
        float deltaTime = clock.restart().asSeconds();

        // Increment game timer (used to release ghosts)
        gameTimer++;

        // Handle events
        Event event;
        while (window.pollEvent(event))
        {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed)
            {
                // Close window with Escape key
                if (Keyboard::isKeyPressed(Keyboard::Key::Escape))
                    window.close();

                // Reset game with R key
                if (Keyboard::isKeyPressed(Keyboard::Key::R))
                {
                    maze.redrawDots();
                    maze.setScore(0);
                    pacman.reset();

                    // Reset all ghosts
                    for (size_t i = 0; i < ghosts.size(); i++) {
                        ghosts[i]->reset();
                        ghosts[i]->setCanLeave(false);
                    }

                    gameTimer = 0;
                    gameActive = true;
                }

                // Test key: Make ghosts frightened with F key
                if (Keyboard::isKeyPressed(Keyboard::Key::F))
                {
                    for (size_t i = 0; i < ghosts.size(); i++) {
                        ghosts[i]->setState(Ghost::FRIGHTENED, 300); // 5 seconds at 60 FPS
                    }
                }

                // Test key: Release all ghosts with L key
                if (Keyboard::isKeyPressed(Keyboard::Key::L))
                {
                    for (size_t i = 0; i < ghosts.size(); i++) {
                        ghosts[i]->setCanLeave(true);
                    }
                }
            }
        }

        // Release ghosts based on timer
        if (gameActive) {
            // Release red ghost after 1 second (60 frames at 60fps)
            if (gameTimer == 60) {
                redGhost.setCanLeave(true);
                cout << "Red ghost released" << endl;
            }
            // Release pink ghost after 3 seconds
            else if (gameTimer == 180) {
                pinkGhost.setCanLeave(true);
                cout << "Pink ghost released" << endl;
            }
            // Release blue ghost after 5 seconds
            else if (gameTimer == 300) {
                blueGhost.setCanLeave(true);
                cout << "Blue ghost released" << endl;
            }
            // Release orange ghost after 7 seconds
            else if (gameTimer == 420) {
                orangeGhost.setCanLeave(true);
                cout << "Orange ghost released" << endl;
            }
        }

        // Handle Pacman input and update if game is active
        if (gameActive) {
            pacman.handleInput();
            pacman.update(deltaTime);
            if (pacman.isPowerMode() && !wasPowerMode) {
                // Pacman just entered power mode, set ghosts to frightened
                for (size_t i = 0; i < ghosts.size(); i++) {
                    ghosts[i]->setState(Ghost::FRIGHTENED, 300); // 5 seconds at 60 FPS
                }
            }
            wasPowerMode = pacman.isPowerMode();

            // Update all ghosts
            for (size_t i = 0; i < ghosts.size(); i++) {

                ghosts[i]->update(deltaTime);

                // Debug output for ghost states (every 120 frames)
                if (gameTimer % 120 == 0) {
                    cout << "Ghost state: " << ghosts[i]->getGhostName()
                        << " at (" << ghosts[i]->getX() << "," << ghosts[i]->getY()
                        << "), Can leave: " << (ghosts[i]->canLeave() ? "YES" : "NO")
                        << ", Has left house: " << (ghosts[i]->hasLeftGhostHouse ? "YES" : "NO")
                        << ", In ghost house: " << (ghosts[i]->isInGhostHouse() ? "YES" : "NO")
                        << ", Direction: " << ghosts[i]->getDirection() << endl;
                }

                // Check for collision with Pacman only if ghost can leave
                if (ghosts[i]->canLeave() && ghosts[i]->checkCollision(pacman.getX(), pacman.getY())) {
                    if (ghosts[i]->isFrightened()) {
                        // Ghost is frightened - Pacman eats ghost
                        pacman.eatGhost();
                        ghosts[i]->getEaten();
                    }
                    else if (!ghosts[i]->isEaten()) {
                        // Ghost is normal - Pacman loses life
                        pacman.loseLife();

                        // Reset all ghosts to their starting positions
                        for (size_t j = 0; j < ghosts.size(); j++) {
                            ghosts[j]->reset();
                            ghosts[j]->setCanLeave(false);
                        }

                        // Reset ghost release timer
                        gameTimer = 0;

                        // Check if game over
                        if (pacman.getLives() <= 0) {
                            gameActive = false;
                        }

                        break;
                    }
                }
            }
        }

        // Update the HUD with current game values
        hud.update();

        // Clear the window
        window.clear(Color::Black);

        // Draw the maze
        maze.draw(window);

        // Draw Pacman
        pacman.draw(window);

        // Draw all ghosts
        for (size_t i = 0; i < ghosts.size(); i++) {
            ghosts[i]->draw(window);
        }

        // Draw the HUD
        hud.draw(window);

        // Display everything
        window.display();
    }

    return 0;
}