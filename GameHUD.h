#ifndef GAMEHUD_H
#define GAMEHUD_H

#include <SFML/Graphics.hpp>
#include "Maze.h"
#include "Pacman.h"
#include <string>
#include <iostream>

using namespace sf;
using namespace std;

class GameHUD {
public:
    // Constructor - load font and set up text elements
    GameHUD(Maze* mazeRef, Pacman* pacmanRef) {
        this->mazeRef = mazeRef;
        this->pacmanRef = pacmanRef;

        // Try to load the font
        if (!font.loadFromFile("fonts/arial.ttf")) {
            // Fallback fonts to try if arial.ttf is not found
            if (!font.loadFromFile("C:/Windows/Fonts/arial.ttf") &&
                !font.loadFromFile("/S ystem/Library/Fonts/Helvetica.ttc")) {
                cout << "Warning: Could not load font. Text will not be displayed." << endl;
                fontLoaded = false;
            }
            else {
                fontLoaded = true;
            }
        }
        else {
            fontLoaded = true;
        }

        // Set up score text
        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(400,710 );

        // Set up lives text
        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(Color::White);
        livesText.setPosition(400, 730);
    }

    // Update the HUD with current game values
    void update() {
        if (!fontLoaded) return;

        // Update score text
        scoreText.setString("SCORE: " + to_string(mazeRef->score));

        // Update lives text
        livesText.setString("LIVES: " + to_string(pacmanRef->getLives()));
    }

    // Draw the HUD to the screen
    void draw(RenderWindow& window) {
        if (!fontLoaded) return;

        window.draw(scoreText);
        window.draw(livesText);
    }

private:
    Maze* mazeRef;
    Pacman* pacmanRef;
    Font font;
    Text scoreText;
    Text livesText;
    bool fontLoaded = false;
};

#endif // GAMEHUD_H