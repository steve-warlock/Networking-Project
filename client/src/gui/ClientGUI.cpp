//
//  ClientGUI.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "../../headers/ClientGUI.hpp"

using namespace std::filesystem;

namespace gui {

ClientGUI::ClientGUI(backend::ClientBackend &backend)
: backend(backend), logger("./client_gui.log"), cursorVisible(true),
cursorBlinkInterval(sf::seconds(0.5f)) {
    try {
        // setup window (font, cursor, path)
        std::string initial_path = current_path().string();
        this -> backend.SetPath(initial_path);
        
        initializeWindow();
        loadFont();
        setupTexts();
        initializeCursor();
        
        logger.log("[DEBUG](ClientGUI::ClientGUI) GUI initialized successfully.");
    }
    catch (const std::exception& e) {
        logger.log("[ERROR](ClientGUI::ClientGUI) An error occured during initialization: " + std::string(e.what()));
    }
}

void ClientGUI::initializeWindow() {
    this -> window.create(sf::VideoMode(1280, 720), "Client RemMux", sf::Style::Resize | sf::Style::Default);
    this -> window.setFramerateLimit(60);
    this -> window.setVerticalSyncEnabled(true);
    logger.log("[DEBUG](ClientGUI::ClientGUI) Window initialized successfully.");
}

void ClientGUI::loadFont() {
    std::string fontPath = path(current_path() / "assets" / "arial.ttf").string();
    if (!this -> font.loadFromFile(fontPath)) {
        logger.log("[ERROR](ClientGUI::loadFont) Failed to load font from path: " + fontPath);
        throw std::runtime_error("Failed to load font from path: " + fontPath);
    }
    logger.log("[DEBUG](ClientGUI::loadFont) Font loaded successfully from path: " + fontPath);
}

void ClientGUI::setupTexts(){
    std::string currentPath = this -> backend.GetPath();
    std::string initialText = currentPath + "> ";
    
    // load font
    this -> inputText.setFont(this -> font);
    this -> outputText.setFont(this -> font);
    
    // set character size
    this -> inputText.setCharacterSize(20);
    this -> outputText.setCharacterSize(20);
    
    // load text color
    this -> inputText.setFillColor(sf::Color::White);
    this -> outputText.setFillColor(sf::Color::White);
    
    // set position
    this -> inputText.setPosition(0, this -> inputYPosition);
    this -> outputText.setPosition(0, 0);
    
    // set String text
    this -> inputText.setString(initialText);
    this -> outputText.setString("");
    
    logger.log("[DEBUG](ClientGui::setupTexts) Text elements configured successfully.");
}

void ClientGUI::initializeCursor() {
    // create cursor line
    this -> cursor.setSize(sf::Vector2f(2,this -> inputText.getCharacterSize()));
    this -> cursor.setFillColor(sf::Color::White);
    
    // position cursor after shell path
    std::string currentPath = this -> backend.GetPath() + "> ";
    const sf::Font* font = this -> inputText.getFont();
    
    // offset for cursor
    float pathOffset = 0;
    for (char c : currentPath) {
        pathOffset += font->getGlyph(c, inputText.getCharacterSize(), false).advance;
    }
    
    // position cursor after shell path
    this -> cursor.setPosition(
                               this -> inputText.getPosition().x + pathOffset,
                               this -> inputText.getPosition().y + 2
                               );
    
    // make the cursor blink
    this -> cursorBlinkClock.restart();
    this -> cursorBlinkInterval = sf::seconds(0.5f);
    this -> cursorVisible = true;
    
    logger.log("[DEBUG](ClientGUI::initializeCursor) Cursor initialized successfully.");
    
}

void ClientGUI::updateCursor() {
    // Get current path with shell prompt
    std::string currentPath = this -> backend.GetPath() + "> ";
    
    // Get font for precise character width calculation
    const sf::Font* font = inputText.getFont();
    
    // Get entire input string
    std::string fullInput = inputText.getString();
    std::string currentInput = fullInput.substr(currentPath.length());
    
    // Ensure cursor position does not exceed input length
    float maxCursorPosition = static_cast<float>(currentInput.length());
    this -> cursorPosition = std::min(std::max(0.0f, this -> cursorPosition), maxCursorPosition);
    
    // temporary text to get the exact position of the last character
    sf::Text tempText;
    tempText.setFont(*font);
    tempText.setCharacterSize(inputText.getCharacterSize());
    tempText.setString(currentPath + currentInput);
    
    // get last character
    float cursorOffset = tempText.findCharacterPos(currentPath.length() + static_cast<int>(cursorPosition)).x;
    
    cursor.setPosition(cursorOffset, this -> inputText.getPosition().y + 3);
    cursor.setSize(sf::Vector2f(2, this -> inputText.getCharacterSize()));
    
    logger.log("[DEBUG](ClientGUI::updateCursor) Cursor positioned at: " +
               std::to_string(cursorOffset) + ", Position: " + std::to_string(cursorPosition));
}

void ClientGUI::updateTerminalDisplay() {
    try {
        
        size_t maxVisibleLines = std::min(this->MAX_VISIBLE_LINES,
                    static_cast<size_t>(this->window.getSize().y / this->inputText.getCharacterSize())
                );
        
        std::string displayText;
        
        size_t startIndex = (terminalLines.size() > maxVisibleLines) ?
                    (terminalLines.size() - maxVisibleLines - scrollPosition) : 0;
        
        // Add output lines
        for (size_t i = startIndex; i < terminalLines.size(); ++i) {
            displayText += (terminalLines[i] + "\n");
        }
        
        displayText.pop_back();
        
        // Update output text
        this -> outputText.setString(displayText);
        this -> outputText.setPosition(0, 0);
        
        // Calculate input Y position
        float outputHeight = this -> outputText.getLocalBounds().height;
        
        float newInputYPosition;
        if (terminalLines.size() == 0) {
            newInputYPosition = 0;
        } else {
            newInputYPosition = std::max(0.0f, outputHeight + 5);
        }
        
        if (std::abs(newInputYPosition - inputYPosition) >= 1.0f) {
            inputYPosition = newInputYPosition;
            this->inputText.setPosition(0, inputYPosition);
            
            logger.log("[DEBUG](ClientGUI::updateTerminalDisplay) Position Updated: " +
                       std::to_string(inputYPosition) +
                       ", Output Height: " + std::to_string(outputHeight));
        }
        
        
        
        // Debug logging
        this -> logger.log("[DEBUG](ClientGUI::updateTerminalDisplay) Input Y Position: " +
                           std::to_string(inputYPosition) +
                           ". Total lines: " + std::to_string(this->terminalLines.size()));
    }
    catch (const std::exception& e) {
        // Log any exception that occurs
        this -> logger.log("[ERROR](ClientGUI::updateTerminalDisplay) Exception: " + std::string(e.what()));
    }
    catch (...) {
        // Log any unknown exception
        this -> logger.log("[ERROR](ClientGUI::updateTerminalDisplay) Unknown exception occurred");
    }
}

void ClientGUI::updateScrollBar() {
    size_t maxVisibleLines = std::min(this -> MAX_VISIBLE_LINES,
                                      static_cast<size_t>(this -> window.getSize().y / this -> inputText.getCharacterSize())
                                      );
    
    // create scrollbar if needed
    if(this -> terminalLines.size() > maxVisibleLines) {
        // scrollbar height proportional to visible area
        float scrollBarHeight = (static_cast<float>(maxVisibleLines) / this -> terminalLines.size()) * this -> window.getSize().y;
        
        // scrollbar position based on scroll position
        float scrollBarPosition = ((this -> terminalLines.size() - maxVisibleLines - this -> scrollPosition) /
                                   static_cast<float>(this -> terminalLines.size())) * this -> window.getSize().y;
        
        // scroll bar position
        this -> scrollBar.setSize(sf::Vector2f(10, scrollBarHeight));
        this -> scrollBar.setPosition(window.getSize().x - 15, scrollBarPosition);
        this -> scrollBar.setFillColor(sf::Color(100,100,100,200));
        
    }
    else {
        this -> scrollBar.setSize(sf::Vector2f(0,0));
    }
    
    logger.log("[DEBUG](ClientGUI::updateScrollBar) Updated scroll bar. Total lines: " + std::to_string(this -> terminalLines.size()) +
               ", Visible lines: " + std::to_string(maxVisibleLines) + ", Scroll position: " + std::to_string(this -> scrollPosition));
    
}

void ClientGUI::scrollUp(int lines) {
    
    size_t maxVisibleLines = std::min(this -> MAX_VISIBLE_LINES,
                                      static_cast<size_t>(this -> window.getSize().y / this -> inputText.getCharacterSize())
                                      );
    
    if(this -> terminalLines.size() > maxVisibleLines) {
        // scroll position to go up to older lines
        this -> scrollPosition = std::min(
                                          this -> scrollPosition + lines,
                                          std::max(0, static_cast<int>(this -> terminalLines.size()) - static_cast<int>(maxVisibleLines))
                                          );
        
        // update display and scrollbar
        updateTerminalDisplay();
        updateScrollBar();
        
        logger.log("[DEBUG](ClientGUI::scrollUp) Scrolled up " +
                   std::to_string(lines) + " lines. Current scroll position: " +
                   std::to_string(this -> scrollPosition));
    }
}

void ClientGUI::scrollDown(int lines) {
    
    // scroll position to down up to newer lines
    if(this -> scrollPosition > 0){
        this -> scrollPosition = std::max(0, this -> scrollPosition - lines);
    
        // update display and scrollbar
        updateTerminalDisplay();
        updateScrollBar();
    
        logger.log("[DEBUG](ClientGUI::scrollDown) Scrolled down " +
                   std::to_string(lines) + " lines. Current scroll position: " +
                   std::to_string(this -> scrollPosition));
    }
}


void ClientGUI::handleSpecialInput(sf::Event event) {
    switch (event.key.code) {
        case sf::Keyboard::Right:
            // Move cursor left, but not before shell path
            this -> cursorPosition = std::min(
                                              static_cast<float>(this -> inputText.getString().getSize() - this -> backend.GetPath().length() - 2.0f),
                                              this -> cursorPosition + 1
                                              );
            break;
            
        case sf::Keyboard::Left:
            // Move cursor right, but not beyond input text
            this -> cursorPosition = std::max(0.0f, this -> cursorPosition - 1.0f);
            break;
            
        default:
            break;
    }
    
    logger.log("[DEBUG](ClientGUI::handleSpecialInput) Processed special input.");
    
    updateCursor();
    
}

void ClientGUI::processNanoInput(const std::string& input, sf::Text& outputText){
    
}

void ClientGUI::processInput(sf::Event event) {
    try {
        
        if (currentMode == editorMode::EDITTING) {
            return;
        }
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode < 128) {
                char inputChar = static_cast<char>(event.text.unicode);
                std::string currentInput = this -> inputText.getString();
                std::string currentPath = this -> backend.GetPath() + "> ";
                
                // Handle Enter
                if (inputChar == '\r' || inputChar == '\n') {
                    std::string command;
                    if (currentInput.length() > currentPath.length()) {
                        command = currentInput.substr(currentPath.length());
                    }
                    
                    if (!command.empty()) {
                        try {
                            addLineToTerminal(currentInput);
                            std::string response = this->backend.sendCommand(command);
                            
                            // check if the command is cd
                            if (command.substr(0, 2) == "cd") {
                                if (response.find("Invalid directory") == std::string::npos &&
                                    response.find("Error") == std::string::npos) {
                                    std::string newPath = response.substr(response.find_last_of('\n') + 1);
                                    this->backend.SetPath(newPath);
                                    addLineToTerminal("Changed directory to: " + newPath);
                                } else {
                                    addLineToTerminal(response);
                                }
                            } else // check if the command is exit
                                if (command == "exit") {
                                    addLineToTerminal(response);
                                    this->window.clear(sf::Color::Black);
                                    this->window.draw(this->outputText);
                                    this -> window.draw(this -> scrollBar);
                                    this->window.display();
                                    sf::sleep(sf::milliseconds(700));
                                    this->window.close();
                                    return;
                                } else if (command == "clear") {
                                    this -> terminalLines.clear();
                                    updateTerminalDisplay();
                                    updateScrollBar();
                                    
                                    std::string newCurrentPath = this->backend.GetPath() + "> ";
                                    this->inputText.setString(newCurrentPath);
                                    this->cursorPosition = 0.0f;
                                    updateCursor();
                                    
                                    return;
                                } else {
                                    // Normal output case for other commands
                                    if (!response.empty()) {
                                        std::istringstream responseStream(response);
                                        std::string line;
                                        while (std::getline(responseStream, line)) {
                                            if (!line.empty()) {
                                                addLineToTerminal(line);
                                            }
                                        }
                                    }
                                }
                            
                            // Update inputText with new path
                            std::string newCurrentPath = this->backend.GetPath() + "> ";
                            this -> inputText.setString(newCurrentPath);
                            this -> cursorPosition = 0.0f;
                            updateCursor();
                        } catch (const std::exception& e) {
                            addLineToTerminal("Error: " + std::string(e.what()));
                            this -> inputText.setString(currentPath);
                            this -> cursorPosition = 0.0f;
                            updateCursor();
                        }
                    }
                }
                // Handle Backspace
                else if (inputChar == '\b') {
                    if (this -> cursorPosition > 0.0f) {
                        if (currentInput.length() > currentPath.length()) {
                            currentInput.erase(currentPath.length() + static_cast<size_t>(this -> cursorPosition) - 1, 1);
                            this -> inputText.setString(currentInput);
                            this -> cursorPosition --;
                        }
                    }
                }
                // Insert characters
                else {
                    if (currentInput.length() < currentPath.length() + 256) {
                        currentInput.insert(currentPath.length() + static_cast<int>(this -> cursorPosition), 1, inputChar);
                        this -> inputText.setString(currentInput);
                        this -> cursorPosition ++;
                    }
                }
                
                updateCursor();
            }
        }
    }
    catch (const std::exception& e) {
        logger.log("[ERROR](ClientGUI::processInput) Exception: " + std::string(e.what()));
        addLineToTerminal("Error: " + std::string(e.what()));
    }
    catch (...) {
        logger.log("[ERROR](ClientGUI::processInput) Unknown exception occurred.");
        addLineToTerminal("An unknown error occurred.");
    }
}


void ClientGUI::addLineToTerminal(const std::string &line) {
    // Trim whitespaces
    std::string trimmedLine = line;
    trimmedLine.erase(0, trimmedLine.find_first_not_of(" \t"));
    trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);
    
    // Only add non-empty lines
    if (!trimmedLine.empty()) {
        // Add line to terminal history
        this -> terminalLines.push_back(trimmedLine);
        
        // Limit terminal history
        if (this -> terminalLines.size() > MAX_HISTORY) {
            this -> terminalLines.erase(
                                        this -> terminalLines.begin(),
                                        this -> terminalLines.begin() + (this -> terminalLines.size() - MAX_HISTORY)
                                        );
        }
        
        // Reset scroll position to bottom
        this -> scrollPosition = 0;
        
        // Update display and scroll bar
        updateTerminalDisplay();
        updateScrollBar();
        
        this -> logger.log("[DEBUG](ClientGUI::addLineToTerminal) Added line. Total lines: " +
                           std::to_string(terminalLines.size()));
    }
}

void ClientGUI::run() {
    
    // smooth trackpad movement
    float trackpadScrollAccumulator = 0.0f;
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }
            
            if (event.type == sf::Event::TextEntered) {
                processInput(event);
            }
            
            if (event.type == sf::Event::KeyPressed) {
                handleSpecialInput(event);
            }
            
            if(event.type == sf::Event::MouseWheelScrolled) {
                trackpadScrollAccumulator += event.mouseWheelScroll.delta;
                if(std::abs(trackpadScrollAccumulator) >= 1.0f) {
                    int scrollLines = static_cast<int>(trackpadScrollAccumulator);
                    if(scrollLines > 0) { // positive means go down
                        scrollUp(1);
                    }
                    else if(scrollLines < 0) { // negative means go up
                        scrollDown(1);
                    }
                }
                trackpadScrollAccumulator = 0.0f;
            }
            
        }
        
        updateCursor();
        updateTerminalDisplay();
        updateScrollBar();
        
        this -> window.clear(sf::Color::Black);
        
        if (!this -> outputText.getString().isEmpty()) {
            this -> window.draw(outputText);
        }
        
        this -> window.draw(this -> inputText);
        
        // Cursor blinking mechanism
        if (this -> cursorBlinkClock.getElapsedTime() >= this -> cursorBlinkInterval) {
            this -> cursorVisible = !this -> cursorVisible;
            this -> cursor.setFillColor(this -> cursorVisible ? sf::Color::White : sf::Color::Transparent);
            this -> cursorBlinkClock.restart();
        }
        
        this -> window.draw(this -> cursor);
        
        if (this -> scrollBar.getSize().y > 0) {
            this -> window.draw(this -> scrollBar);
        }
        
        this -> window.display();
    }
}

}
