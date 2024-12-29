//
//  ClientGUI.h
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#pragma once

#include "./ClientBackend.hpp"
#include "./Logger.hpp"

// SFML
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

//std
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <filesystem>

namespace gui {

enum class editorMode {
    NORMAL,
    EDITTING
};

class ClientGUI {
    
public:
    explicit ClientGUI(backend::ClientBackend &backend);
    void run();
    
    // deactivate copy constructors and assign operator
    ClientGUI(const ClientGUI&) = delete;
    ClientGUI& operator = (const ClientGUI&) = delete;
    
private:
    backend::ClientBackend &backend;
    sf::RenderWindow window;
    logs::Logger guiLogger;
    
    // gui resourses window with input and output text
    sf::Font font;
    sf::Text inputText;
    sf::Text outputText;
    
    // terminal
    std::vector<std::string> terminalLines;
    size_t scrollOffset = 0;
    sf::RectangleShape scrollBar;
    int scrollPosition = 0;
    const size_t MAX_VISIBLE_LINES = 30;
    const size_t MAX_HISTORY = 1000;
    float inputYPosition = 0.0f;
    
    // cursor
    sf::RectangleShape cursor;
    sf::Clock cursorBlinkClock;
    bool cursorVisible = true;
    sf::Time cursorBlinkInterval;
    float cursorPosition = 0;
    
    
    // editor member text for nano
    editorMode currentMode = editorMode::NORMAL;
    std::vector<std::string> editorLines;
    std::string currentEditingFile;
    
    void initializeWindow();
    void loadFont();
    void setupTexts();
    
    
    // Terminal methods
    void updateTerminalDisplay();
    void updateScrollBar();
    void scrollUp(int lines = 1);
    void scrollDown(int lines = 1);
    void addLineToTerminal(const std::string& line);
    
    // cursor methods
    void initializeCursor();
    void updateCursor();
    
    // Process input methods
    void processInput(sf::Event event);
    void handleSpecialInput(sf::Event event);
    
    // editor mode methods
    void enterNanoEditorMode(const std::string& filecontent);
    std::vector<std::string>splitFileContent(const std::string& content);
    void processNanoInput(sf::Event event);
    void exitNanoEditorMode();
    void refreshNanoDisplay();
    
    // command history
    std::vector<std::string> commandHistory;
    size_t currentHistoryIndex = -1;
    void navigateCommandHistory(bool goUp);
    
};

}
