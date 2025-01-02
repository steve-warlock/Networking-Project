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
: backend(backend), guiLogger("./client_gui.log"), cursorVisible(true),
cursorBlinkInterval(sf::seconds(0.5f)) {
    try {
        // setup window (font, cursor, path)
        std::string initial_path = current_path().string();
        this -> backend.SetPath(initial_path);
        
        initializeWindow();
        loadFont();
        setupTexts();
        initializeCursor();
        
        guiLogger.log("[DEBUG](ClientGUI::ClientGUI) GUI initialized successfully.");
    }
    catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::ClientGUI) An error occured during initialization: " + std::string(e.what()));
    }
}

void ClientGUI::initializeWindow() {
    this -> window.create(sf::VideoMode(1280, 720), "Client RemMux", sf::Style::Resize | sf::Style::Default);
    this -> window.setFramerateLimit(60);
    this -> window.setVerticalSyncEnabled(true);
    guiLogger.log("[DEBUG](ClientGUI::ClientGUI) Window initialized successfully.");
}

void ClientGUI::loadFont() {
    std::string fontPath = path(current_path() / "assets" / "arial.ttf").string();
    if (!this -> font.loadFromFile(fontPath)) {
        guiLogger.log("[ERROR](ClientGUI::loadFont) Failed to load font from path: " + fontPath);
        throw std::runtime_error("Failed to load font from path: " + fontPath);
    }
    guiLogger.log("[DEBUG](ClientGUI::loadFont) Font loaded successfully from path: " + fontPath);
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
    
    guiLogger.log("[DEBUG](ClientGui::setupTexts) Text elements configured successfully.");
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
    
    guiLogger.log("[DEBUG](ClientGUI::initializeCursor) Cursor initialized successfully.");
    
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
    
    guiLogger.log("[DEBUG](ClientGUI::updateCursor) Cursor positioned at: " +
                  std::to_string(cursorOffset) + ", Position: " + std::to_string(cursorPosition));
}

void ClientGUI::updateTerminalDisplay() {
    try {
        
        size_t maxVisibleLines = std::min(this -> MAX_VISIBLE_LINES,
                                          static_cast<size_t>(this -> window.getSize().y / this -> inputText.getCharacterSize())
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
            this -> inputText.setPosition(0, inputYPosition);
            
            guiLogger.log("[DEBUG](ClientGUI::updateTerminalDisplay) Position Updated: " +
                          std::to_string(inputYPosition) +
                          ", Output Height: " + std::to_string(outputHeight));
        }
        
        
        
        // Debug logging
        this -> guiLogger.log("[DEBUG](ClientGUI::updateTerminalDisplay) Input Y Position: " +
                              std::to_string(inputYPosition) +
                              ". Total lines: " + std::to_string(this -> terminalLines.size()));
    }
    catch (const std::exception& e) {
        // Log any exception that occurs
        this -> guiLogger.log("[ERROR](ClientGUI::updateTerminalDisplay) Exception: " + std::string(e.what()));
    }
    catch (...) {
        // Log any unknown exception
        this -> guiLogger.log("[ERROR](ClientGUI::updateTerminalDisplay) Unknown exception occurred");
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
    
    guiLogger.log("[DEBUG](ClientGUI::updateScrollBar) Updated scroll bar. Total lines: " + std::to_string(this -> terminalLines.size()) +
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
        
        guiLogger.log("[DEBUG](ClientGUI::scrollUp) Scrolled up " +
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
        
        guiLogger.log("[DEBUG](ClientGUI::scrollDown) Scrolled down " +
                      std::to_string(lines) + " lines. Current scroll position: " +
                      std::to_string(this -> scrollPosition));
    }
}

void ClientGUI::navigateCommandHistory(bool goUp) {
    std::string currentPath = this -> backend.GetPath() + "> ";
    
    if (this -> commandHistory.empty()) {
        guiLogger.log("[DEBUG](ClientGUI::navigateCommandHistory) No commands in history.");
        return;
    }
    
    if (goUp) {
        if (this -> currentHistoryIndex == -1) {
            this -> currentHistoryIndex = this -> commandHistory.size() - 1;
        } else if (this -> currentHistoryIndex > 0) {
            this -> currentHistoryIndex--;
        }
    } else {
        if (this -> currentHistoryIndex == -1) {
            return;
        }
        
        if (this -> currentHistoryIndex < this -> commandHistory.size() - 1) {
            this -> currentHistoryIndex++;
        } else {
            this -> currentHistoryIndex = -1;
        }
    }
    
    if (this -> currentHistoryIndex != -1) {
        std::string fullCommand = currentPath + this -> commandHistory[this -> currentHistoryIndex];
        this -> inputText.setString(fullCommand);
        
        this -> cursorPosition = this -> commandHistory[this -> currentHistoryIndex].length();
        
        guiLogger.log("[DEBUG](ClientGUI::navigateCommandHistory) Selected command: '" +
                      this -> commandHistory[this -> currentHistoryIndex] +
                      "'. Index: " + std::to_string(this -> currentHistoryIndex));
    } else {
        this -> inputText.setString(currentPath);
        this -> cursorPosition = 0;
        
        guiLogger.log("[DEBUG](ClientGUI::navigateCommandHistory) Reset to initial state.");
    }
    
    updateCursor();
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
            
        case sf::Keyboard::Up:
            navigateCommandHistory(true); // true -> previous commands
            break;
            
        case sf::Keyboard::Down:
            navigateCommandHistory(false); // false -> next commands
            break;
            
        default:
            break;
    }
    
    guiLogger.log("[DEBUG](ClientGUI::handleSpecialInput) Processed special input.");
    
    updateCursor();
    
}

// nano
void ClientGUI::processNanoInput(sf::Event event) {
    // Exit with Ctrl+X
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::X &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
        
        guiLogger.log("[INFO](ClientGUI::processNanoInput) Exiting nano editor.");
        exitNanoEditorMode();
        return;
    }
    
    // Save with Ctrl+O
    if (event.type == sf::Event::KeyPressed &&
        event.key.code == sf::Keyboard::O &&
        sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
        
        guiLogger.log("[INFO](ClientGUI::processNanoInput) Saving file.");
        this->nanoCursor.column = 0;
        this->nanoCursor.line = 0;
        saveNanoFile();
        return;
    }
    
    // Arrow key navigation
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Up:
                if (this -> nanoCursor.line > 0) {
                    this -> nanoCursor.line--;
                    this -> nanoCursor.column = std::min(this -> nanoCursor.column,
                                                         this->editorLines[this -> nanoCursor.line].length());
                }
                break;
                
            case sf::Keyboard::Down:
                if (this -> nanoCursor.line < this->editorLines.size() - 1) {
                    this -> nanoCursor.line++;
                    this -> nanoCursor.column = std::min(this -> nanoCursor.column,
                                                         this -> editorLines[this -> nanoCursor.line].length());
                }
                break;
                
            case sf::Keyboard::Left:
                if (this -> nanoCursor.column > 0) {
                    this -> nanoCursor.column--;
                }
                else if (this -> nanoCursor.line > 0) {
                    this -> nanoCursor.line--;
                    this -> nanoCursor.column = this->editorLines[this -> nanoCursor.line].length();
                }
                break;
                
            case sf::Keyboard::Right:
                if (this -> nanoCursor.column < this->editorLines[this -> nanoCursor.line].length()) {
                    this -> nanoCursor.column++;
                }
                else if (this -> nanoCursor.line < this -> editorLines.size() - 1) {
                    this -> nanoCursor.line++;
                    this -> nanoCursor.column = 0;
                }
                break;
                
            default:
                break;
        }
        
        refreshNanoDisplay();
        return;
    }
    
    // Text input processing
    if (event.type == sf::Event::TextEntered) {
        char inputChar = static_cast<char>(event.text.unicode);
        
        // Ensure at least one line exists
        if (this -> editorLines.empty()) {
            this -> editorLines.push_back("");
            this -> nanoCursor = {0, 0};
        }
        
        switch (inputChar) {
            case '\r':  // Enter
            case '\n': {
                // Split current line at cursor position
                std::string& currentLine = this -> editorLines[this -> nanoCursor.line];
                std::string secondPart = currentLine.substr(this -> nanoCursor.column);
                currentLine = currentLine.substr(0, this -> nanoCursor.column);
                
                // Insert new line
                this->editorLines.insert(
                                         this->editorLines.begin() + this -> nanoCursor.line + 1,
                                         secondPart
                                         );
                
                // Move cursor to start of new line
                nanoCursor.line++;
                nanoCursor.column = 0;
                break;
            }
                
            case '\b':  // Backspace
                if (nanoCursor.column > 0) {
                    // Remove character before cursor
                    this->editorLines[nanoCursor.line].erase(
                                                             nanoCursor.column - 1, 1
                                                             );
                    nanoCursor.column--;
                }
                else if (nanoCursor.line > 0) {
                    // Merge with previous line
                    size_t prevLineLength = this->editorLines[nanoCursor.line - 1].length();
                    this->editorLines[nanoCursor.line - 1] +=
                    this->editorLines[nanoCursor.line];
                    
                    // Remove current line
                    this->editorLines.erase(
                                            this->editorLines.begin() + nanoCursor.line
                                            );
                    
                    // Move cursor
                    nanoCursor.line--;
                    nanoCursor.column = prevLineLength;
                }
                break;
                
            default:
                // Printable characters
                if (inputChar >= 32 && inputChar <= 126) {
                    // Insert character at cursor position
                    this->editorLines[nanoCursor.line].insert(
                                                              nanoCursor.column, 1, inputChar
                                                              );
                    nanoCursor.column++;
                }
                break;
        }
        
        // Redraw
        refreshNanoDisplay();
    }
}

void ClientGUI::saveNanoFile() {
    try {
        // Prepare file content
        std::string fileContent;
        for (const auto& line : this->editorLines) {
            fileContent += line + "\n";
        }
        
        // Open local file for writing
        std::ofstream file(this -> currentEditingFile);
        if (!file.is_open()) {
            throw std::runtime_error("Couldn't open file for writing");
        }
        
        // Write content
        file << fileContent;
        file.close();
        
        // Log and confirm
        guiLogger.log("[INFO](ClientGUI::saveNanoFile) File saved: " +
                      this->currentEditingFile);
        
        this -> savedMessage = "File Saved!";
        
        refreshNanoDisplay();
    }
    catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::saveNanoFile) Save failed: " +
                      std::string(e.what()));
        
        this -> savedMessage = "Save Failed!";
    }
}

std::vector<std::string> ClientGUI::wrapLines(const std::string& originalLine, float maxWidth) {
    std::vector<std::string> wrappedLines;
    sf::Text testText;
    testText.setFont(this->font);
    testText.setCharacterSize(20);
    
    std::string remainingText = originalLine;
    while (!remainingText.empty()) {
        std::string wrappedPart;
        
        // longest part of the line
        for (size_t j = remainingText.length(); j > 0; --j) {
            testText.setString(remainingText.substr(0, j));
            if (testText.getLocalBounds().width <= maxWidth) {
                wrappedPart = remainingText.substr(0, j);
                remainingText = remainingText.substr(j);
                break;
            }
        }
        
        // only the first character
        if (wrappedPart.empty()) {
            wrappedPart = remainingText.substr(0, 1);
            remainingText = remainingText.substr(1);
        }
        
        wrappedLines.push_back(wrappedPart);
    }
    
    return wrappedLines;
}

void ClientGUI::refreshNanoDisplay() {
    // Log entry point
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Starting nano display refresh");
    
    // Prepare content text configuration
    sf::Text contentText;
    contentText.setFont(this->font);
    contentText.setCharacterSize(20);
    contentText.setFillColor(sf::Color::White);
    
    // Determine visible lines based on window size
    size_t maxVisibleLines = (window.getSize().y - 100) / 25;
    float maxWidth = window.getSize().x - 30;
    std::vector<std::string> fullWrappedLines;
    for (size_t i = nanoCursor.scrollOffset;
         i < std::min(nanoCursor.scrollOffset + maxVisibleLines, this->editorLines.size());
         ++i) {
        std::vector<std::string> currentfullWrappedLines = wrapLines(this->editorLines[i], maxWidth);
        fullWrappedLines.insert(fullWrappedLines.end(), currentfullWrappedLines.begin(), currentfullWrappedLines.end());
    }
    
    
    // Log initial scroll state
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Initial scroll state : Line: " + std::to_string(nanoCursor.line) +
                  ", Scroll Offset: " + std::to_string(nanoCursor.scrollOffset) +
                  ", Max Visible Lines: " + std::to_string(maxVisibleLines));
    
    // Adjust scroll offset if cursor is out of view
    if (nanoCursor.line < nanoCursor.scrollOffset) {
        nanoCursor.scrollOffset = nanoCursor.line;
        guiLogger.log("[INFO](ClientGUI::refreshNanoDisplay) Adjusted scroll offset down");
    }
    if (nanoCursor.line >= nanoCursor.scrollOffset + maxVisibleLines) {
        nanoCursor.scrollOffset = nanoCursor.line - maxVisibleLines + 1;
        guiLogger.log("[INFO](ClientGUI::refreshNanoDisplay) Adjusted scroll offset up");
    }
    
    // Log updated scroll state
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Updated scroll state : Line: " + std::to_string(nanoCursor.line) +
                  ", Scroll Offset: " + std::to_string(nanoCursor.scrollOffset));
    
    // Prepare full line text
    std::string fullLineText = this->editorLines[nanoCursor.line];
    contentText.setString(fullLineText);
    
    // Calculate cursor position more precisely
    sf::Text cursorPositionText;
    cursorPositionText.setFont(this->font);
    cursorPositionText.setCharacterSize(20);
    
    // Get text before cursor
    std::string textBeforeCursor = fullLineText.substr(0, nanoCursor.column);
    cursorPositionText.setString(textBeforeCursor);
    
    // Find exact cursor position
    sf::Vector2f cursorPos = cursorPositionText.findCharacterPos(nanoCursor.column);
    
    // Create cursor shape
    sf::RectangleShape cursor;
    cursor.setSize(sf::Vector2f(2, 20)); // Thin vertical line
    cursor.setFillColor(sf::Color::White);
    
    // Adjust cursor position with slight horizontal offset
    cursor.setPosition(
                       cursorPos.x + 10, // Add slight offset to match terminal prompt
                       50 + (nanoCursor.line - nanoCursor.scrollOffset) * 25
                       );
    
    // Create render texture to reduce flickering
    sf::RenderTexture renderTexture;
    renderTexture.create(window.getSize().x, window.getSize().y);
    renderTexture.clear(sf::Color::Black);
    
    // Header text
    sf::Text headerText;
    headerText.setFont(this->font);
    headerText.setCharacterSize(20);
    headerText.setFillColor(sf::Color::White);
    headerText.setString("nano: " + currentEditingFile);
    headerText.setPosition(10, 10);
    renderTexture.draw(headerText);
    
    // Render file content
    float yPosition = 50;
    for (size_t i = 0; i < fullWrappedLines.size() && i < maxVisibleLines; ++i) {
        contentText.setString(fullWrappedLines[i]);
        contentText.setPosition(10, yPosition);
        
        if (i == nanoCursor.line - nanoCursor.scrollOffset) {
            sf::RectangleShape lineHighlight;
            lineHighlight.setSize(sf::Vector2f(window.getSize().x, 25));
            lineHighlight.setPosition(0, yPosition);
            lineHighlight.setFillColor(sf::Color(50, 50, 50, 100));
            renderTexture.draw(lineHighlight);
        }
        
        renderTexture.draw(contentText);
        yPosition += 25;
    }
    
    // Log rendering details
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Rendered lines: " +
                  std::to_string(fullWrappedLines.size()) +
                  ", Total lines: " + std::to_string(this->editorLines.size()));
    
    // Footer text
    sf::Text footerText;
    footerText.setFont(this->font);
    footerText.setCharacterSize(16);
    footerText.setFillColor(sf::Color::Green);
    static std::string lastSavedMessage;
    static sf::Clock messageClock;
    
    if (!savedMessage.empty()) {
        lastSavedMessage = this -> savedMessage;
        messageClock.restart();  // reset clock after each message
        this -> savedMessage.clear();    // clear old message
    }
    
    // hold the text for 2 seconds
    if (messageClock.getElapsedTime().asSeconds() <= 2.0f && !lastSavedMessage.empty()) {
        footerText.setFillColor(sf::Color::Green);
        footerText.setString("^O Save   ^X Exit      " + lastSavedMessage);
    } else {
        lastSavedMessage.clear();
        footerText.setFillColor(sf::Color::Green);
        footerText.setString("^O Save   ^X Exit");
    }
    
    footerText.setPosition(10, this->window.getSize().y - 30);
    renderTexture.draw(footerText);
    
    // get sprite textures
    sf::Sprite sprite(renderTexture.getTexture());
    
    // Draw cursor
    if (nanoCursor.line >= nanoCursor.scrollOffset && nanoCursor.line < nanoCursor.scrollOffset + maxVisibleLines) {
        sf::Text cursorText;
        cursorText.setFont(this->font);
        cursorText.setCharacterSize(20);
        cursorText.setString(this->editorLines[nanoCursor.line].substr(0, nanoCursor.column));
        sf::Vector2f cursorPos = cursorText.findCharacterPos(nanoCursor.column);
        
        sf::RectangleShape cursor;
        cursor.setSize(sf::Vector2f(2, 20)); // Thin vertical line
        cursor.setFillColor(sf::Color::White);
        cursor.setPosition(10 + cursorPos.x, 50 + (nanoCursor.line - nanoCursor.scrollOffset) * 25);
        renderTexture.draw(cursor);
    }
    
    // Finalize rendering
    renderTexture.display();
    this -> window.clear();
    // Display texture
    this -> window.draw(sprite);
    this -> window.display();
    
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Nano display refresh complete");
}

void ClientGUI::enterNanoEditorMode(const std::string& fileContent, const std::string& fileName) {
    // Log entry into nano editor mode
    guiLogger.log("[INFO](ClientGUI::enterNanoEditorMode) Entering Nano Editor Mode.");
    
    // reset the cursor to 0
    this -> nanoCursor = {0, 0, 0};
    
    // Set current mode
    this->currentMode = editorMode::EDITTING;
    
    // hide terminal cursor
    this -> cursor.setSize(sf::Vector2f(0, 0));
    
    // Save filename
    this->currentEditingFile = fileName;
    
    // Reset editor lines
    this->editorLines.clear();
    
    // Split content into lines
    if(!fileContent.empty()) {
        std::istringstream contentStream(fileContent);
        std::string line;
        while (std::getline(contentStream, line)) {
            this->editorLines.push_back(line);
        }
    } else {
        this->editorLines.push_back("");
    }
    
    // Logging
    guiLogger.log("[DEBUG](ClientGUI::enterNanoEditorMode) Loaded " +
                  std::to_string(this->editorLines.size()) + " lines.");
    
    refreshNanoDisplay();
}

void ClientGUI::exitNanoEditorMode() {
    guiLogger.log("[INFO](ClientGUI::exitNanoEditor) Exiting Nano Editor Mode.");
    
    // Reset to normal mode
    this->currentMode = editorMode::NORMAL;
    
    // reset terminal cursor state
    this -> cursor.setSize(sf::Vector2f(2, this -> inputText.getCharacterSize()));
    
    // Reset editor state
    this->editorLines.clear();
    this->currentEditingFile = "";
    
    // Restore terminal state
    std::string currentPath = this->backend.GetPath() + "> ";
    this->inputText.setString(currentPath);
    
    guiLogger.log("[DEBUG](ClientGUI::exitNanoEditor) Nano editor state reset.");
}

// pane functions
void ClientGUI::createNewPane(SplitType splitType){
    if (panes.size() >= 4) {
        guiLogger.log("[WARN](ClientGUI::createNewPane) Maximum number of panes reached.");
        return;
    }
    
    // create the current pane
    if (panes.empty()) {
       // First pane is added by default
        Pane firstPane;
        firstPane.terminalLines = this->terminalLines;
        firstPane.currentPath = this->backend.GetPath();
        firstPane.currentInput = this->inputText.getString();
        firstPane.scrollPosition = this->scrollPosition;
        firstPane.splitType = splitType;
        
        // text for input and output
        firstPane.inputText.setFont(this->font);
        firstPane.outputText.setFont(this->font);
        firstPane.inputText.setCharacterSize(20);
        firstPane.outputText.setCharacterSize(20);
        firstPane.inputText.setFillColor(sf::Color::White);
        firstPane.outputText.setFillColor(sf::Color::White);
        
        // Cursor initialization
        firstPane.cursor.setSize(sf::Vector2f(2, firstPane.inputText.getCharacterSize()));
        firstPane.cursor.setFillColor(sf::Color::White);
        firstPane.cursorBlinkClock.restart();

       panes.push_back(firstPane);
    }

    // add the new pane
    Pane newPane = panes[0];
    newPane.splitType = splitType;
    panes.push_back(newPane);

    // set the index of the new pane
    this -> currentPaneIndex = panes.size() - 1;
    
    updatePaneBounds();
    
    // reset terminal
    this->terminalLines.clear();
    this->backend.SetPath(newPane.currentPath);
    this->inputText.setString(newPane.currentPath + "> ");
    this->scrollPosition = 0;
    
    guiLogger.log("[DEBUG](ClientGUI::createNewPane) Created new pane. Total panes: " + std::to_string(panes.size()) + ", Split type: " + (splitType == SplitType::HORIZONTAL ? "Horizontal" : "Vertical"));
}

void ClientGUI::updatePaneBounds(){
    sf::Vector2u windowSize = window.getSize();
    size_t paneCount = panes.size();
    
    switch(paneCount) {
        case 2: {
            // Split in half
            if (panes[0].splitType == SplitType::HORIZONTAL) {
                // Split orizontal
                panes[0].bounds = sf::FloatRect(0, 0, windowSize.x, windowSize.y / 2);
                panes[1].bounds = sf::FloatRect(0, windowSize.y / 2, windowSize.x, windowSize.y / 2);
            } else {
                // Split vertical
                panes[0].bounds = sf::FloatRect(0, 0, windowSize.x / 2, windowSize.y);
                panes[1].bounds = sf::FloatRect(windowSize.x / 2, 0, windowSize.x / 2, windowSize.y);
            }
            break;
        }
            
        case 3: {
            // first split + a supposed third pane
            if (panes[0].splitType == SplitType::HORIZONTAL) {
                // Split orizontal + third pane
                panes[0].bounds = sf::FloatRect(0, 0, windowSize.x, windowSize.y / 2);
                panes[1].bounds = sf::FloatRect(0, windowSize.y / 2, windowSize.x / 2, windowSize.y / 2);
                panes[2].bounds = sf::FloatRect(windowSize.x / 2, windowSize.y / 2, windowSize.x / 2, windowSize.y / 2);
            } else {
                // Split vertical + third pane
                panes[0].bounds = sf::FloatRect(0, 0, windowSize.x / 2, windowSize.y);
                panes[1].bounds = sf::FloatRect(windowSize.x / 2, 0, windowSize.x / 2, windowSize.y / 2);
                panes[2].bounds = sf::FloatRect(windowSize.x / 2, windowSize.y / 2, windowSize.x / 2, windowSize.y / 2);
            }
            break;
        }
            
        case 4: {
            // all 4 panes
            panes[0].bounds = sf::FloatRect(0, 0, windowSize.x / 2, windowSize.y / 2);
            panes[1].bounds = sf::FloatRect(windowSize.x / 2, 0, windowSize.x / 2, windowSize.y / 2);
            panes[2].bounds = sf::FloatRect(0, windowSize.y / 2, windowSize.x / 2, windowSize.y / 2);
            panes[3].bounds = sf::FloatRect(windowSize.x / 2, windowSize.y / 2, windowSize.x / 2, windowSize.y / 2);
            break;
        }
    }
    
    // Log pentru debugging
    guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Updated bounds for " +
                  std::to_string(paneCount) + " panes.");
}

void ClientGUI::updatePaneTerminalDisplay(Pane& pane) {
    // available height
    float availableHeight = pane.bounds.height;
    float characterHeight = 16;
    size_t maxVisibleLines = static_cast<size_t>(availableHeight / characterHeight);
    
    // line limiting for the size
    std::vector<std::string> visibleLines = pane.terminalLines;
    if (visibleLines.size() > maxVisibleLines) {
        visibleLines = std::vector<std::string>(
                                                visibleLines.end() - maxVisibleLines,
                                                visibleLines.end()
                                                );
    }
    
    // convert line to text
    std::string displayText;
    for (const auto& line : visibleLines) {
        displayText += line + "\n";
    }
    if (!displayText.empty()) {
        displayText.pop_back(); // get rid of the last newline
    }
    
    // Set the text
    pane.outputText.setFont(this->font);
    pane.outputText.setCharacterSize(16);
    pane.outputText.setFillColor(sf::Color::White);
    pane.outputText.setString(displayText);
    pane.outputText.setPosition(pane.bounds.left, pane.bounds.top);
    
    // Set the input line
    std::string currentPath = pane.currentPath;
    pane.inputText.setFont(this->font);
    pane.inputText.setCharacterSize(16);
    pane.inputText.setFillColor(sf::Color::White);
    pane.inputText.setString(currentPath + "> ");
    pane.inputText.setPosition(
                          pane.bounds.left,
                          pane.bounds.top + pane.bounds.height - inputText.getCharacterSize()
                          );
    
    updatePaneBounds();
    
    guiLogger.log("[DEBUG](ClientGUI::updatePaneTerminalDisplay) Pane updated. Total lines: " +
                      std::to_string(pane.terminalLines.size()) +
                      ", Visible lines: " + std::to_string(visibleLines.size()));
    
}

void ClientGUI::updatePaneScrollBar(Pane& pane) {
    // get the height of the pane
    float availableHeight = pane.bounds.height;
    float characterHeight = inputText.getCharacterSize();
    size_t maxVisibleLines = static_cast<size_t>(availableHeight / characterHeight);

    // check if the scrollbar is required
    if (pane.terminalLines.size() > maxVisibleLines) {
        // get the height of the scrollbar
        float scrollBarHeight = (static_cast<float>(maxVisibleLines) / pane.terminalLines.size()) * pane.bounds.height;
        
        // compute the offset of the scrollbar
        float scrollBarPosition = pane.bounds.top +
            ((pane.terminalLines.size() - maxVisibleLines - pane.scrollPosition) /
             static_cast<float>(pane.terminalLines.size())) * pane.bounds.height;

        // Set the scrollbar itself
        scrollBar.setSize(sf::Vector2f(10, scrollBarHeight));
        scrollBar.setPosition(
            pane.bounds.left + pane.bounds.width - 15,
            scrollBarPosition
        );
        scrollBar.setFillColor(sf::Color(100, 100, 100, 200));
    } else {
        // Hide the scrollbar
        scrollBar.setSize(sf::Vector2f(0, 0));
    }
}

void ClientGUI::updatePaneCursor(Pane& pane) {
    // pane's path
    std::string currentPath = pane.currentPath + "> ";
    
    // get the font for width computation
    const sf::Font* font = inputText.getFont();
    
    // get the full input
    std::string fullInput = pane.currentInput;
    std::string currentInput = fullInput.substr(currentPath.length());
    
    // the cursor does not exceed the inputs length
    float maxCursorPosition = static_cast<float>(currentInput.length());
    
    // tempText to get the last character's pos
    sf::Text tempText;
    tempText.setFont(*font);
    tempText.setCharacterSize(inputText.getCharacterSize());
    tempText.setString(currentPath + currentInput);
    
    // get the last input character
    float cursorOffset = tempText.findCharacterPos(currentPath.length() + static_cast<int>(cursorPosition)).x;
    
    // cursor set
    cursor.setPosition(
        pane.bounds.left + cursorOffset,
        pane.bounds.top + pane.bounds.height - inputText.getCharacterSize() + 3
    );
    cursor.setSize(sf::Vector2f(2, inputText.getCharacterSize()));
    
    // logger
    guiLogger.log("[DEBUG] Pane cursor positioned at: " +
                  std::to_string(cursorOffset));
}

void ClientGUI::switchPane(int direction) {
    if (panes.empty()) return;
    
    panes[currentPaneIndex].terminalLines = this->terminalLines;
    panes[currentPaneIndex].currentPath = this->backend.GetPath();
    panes[currentPaneIndex].currentInput = this->inputText.getString();
    panes[currentPaneIndex].scrollPosition = this->scrollPosition;
    
    currentPaneIndex = (currentPaneIndex + direction + panes.size()) % panes.size();
    
    Pane& newPane = panes[currentPaneIndex];
    this->terminalLines = newPane.terminalLines;
    this->backend.SetPath(newPane.currentPath);
    this->inputText.setString(newPane.currentInput);
    this->scrollPosition = newPane.scrollPosition;
}

void ClientGUI::closeCurrentPane() {
    if (panes.size() <= 1) {
        
        this->terminalLines.clear();
        std::string terminal_path = backend.GetPath();
        this->backend.SetPath(terminal_path);
        this->inputText.setString(backend.GetPath() + "> ");
        this->scrollPosition = 0;
        
        panes.clear();
        currentPaneIndex = 0;
    } else {
        panes.erase(panes.begin() + currentPaneIndex);
        
        updatePaneBounds();
        
        currentPaneIndex = std::min(currentPaneIndex, panes.size() - 1);
        
        Pane& remainingPane = panes[currentPaneIndex];
        this->terminalLines = remainingPane.terminalLines;
        this->backend.SetPath(remainingPane.currentPath);
        this->inputText.setString(remainingPane.currentInput);
        this->scrollPosition = remainingPane.scrollPosition;
    }
}

void ClientGUI::processPaneInput(sf::Event event, Pane& currentPane) {
    // Check if the event is text input
    if (event.type == sf::Event::TextEntered) {
        // Ensure we're dealing with a printable character
        if (event.text.unicode < 128) {
            char inputChar = static_cast<char>(event.text.unicode);
            
            // Construct current path with prompt
            std::string currentPath = currentPane.currentPath + "> ";
            std::string currentInput = currentPane.currentInput;
            
            // Get reference to cursor position
            float& cursorPosition = currentPane.cursorPosition;
            
            // Handle Enter key (execute command)
            if (inputChar == '\r' || inputChar == '\n') {
                // Extract command from input
                std::string command = currentInput.substr(currentPath.length());
                
                if (!command.empty()) {
                    // Send command to backend
                    std::string response = this->backend.sendCommand(command);
                    
                    // Add command to terminal lines
                    currentPane.terminalLines.push_back(currentInput);
                    
                    // Process response
                    std::istringstream responseStream(response);
                    std::string line;
                    while (std::getline(responseStream, line)) {
                        currentPane.terminalLines.push_back(line);
                    }
                    
                    // Reset input and cursor
                    currentPane.currentInput = currentPath;
                    currentPane.inputText.setString(currentPath);
                    cursorPosition = 0.0f;
                }
            }
            // Handle Backspace
            else if (inputChar == '\b') {
                if (cursorPosition > 0) {
                    // Remove character before cursor
                    currentInput.erase(currentPath.length() + cursorPosition - 1, 1);
                    currentPane.currentInput = currentInput;
                    currentPane.inputText.setString(currentInput);
                    cursorPosition--;
                }
            }
            // Handle character input
            else {
                // Limit input length
                if (currentInput.length() < currentPath.length() + 256) {
                    // Insert character at cursor position
                    currentInput.insert(currentPath.length() + cursorPosition, 1, inputChar);
                    currentPane.currentInput = currentInput;
                    currentPane.inputText.setString(currentInput);
                    cursorPosition++;
                }
            }
        }
    }
}

void ClientGUI::handlePaneSpecialInput(sf::Event event, Pane& currentPane) {
    // Check if a key is pressed
    if (event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
            case sf::Keyboard::Right:
                // Move cursor right, but not beyond input text
                currentPane.cursorPosition = std::min(
                    static_cast<float>(currentPane.currentInput.length() -
                    (currentPane.currentPath + "> ").length()),
                    currentPane.cursorPosition + 1.0f
                );
                break;
                
            case sf::Keyboard::Left:
                // Move cursor left, but not beyond start
                currentPane.cursorPosition = std::max(0.0f, currentPane.cursorPosition - 1.0f);
                break;
                
            case sf::Keyboard::Up:
                // Navigate to previous commands
                navigatePaneCommandHistory(currentPane, true);
                break;
                
            case sf::Keyboard::Down:
                // Navigate to next commands
                navigatePaneCommandHistory(currentPane, false);
                break;
                
            default:
                break;
        }
        
        // Log cursor movement for debugging
        guiLogger.log("[DEBUG](ClientGUI::handlePaneSpecialInput) Pane special input made");
    }
}

void ClientGUI::navigatePaneCommandHistory(Pane& currentPane, bool goUp) {
    // Get current path with prompt
    std::string currentPath = currentPane.currentPath + "> ";
    
    // Check if command history is empty
    if (currentPane.paneCommandHistory.empty()) {
        guiLogger.log("[DEBUG](ClientGUI::navigatePaneCommandHistory) No commands in pane history.");
        return;
    }
    
    // Navigate through command history
    if (goUp) {
        // If no index selected, start from the last command
        if (currentPane.paneCommandHistoryIndex == -1) {
            currentPane.paneCommandHistoryIndex = currentPane.paneCommandHistory.size() - 1;
        }
        // Move to previous command if possible
        else if (currentPane.paneCommandHistoryIndex > 0) {
            currentPane.paneCommandHistoryIndex--;
        }
    } else {
        // If no index selected, do nothing
        if (currentPane.paneCommandHistoryIndex == -1) {
            return;
        }
        
        // Move to next command
        if (currentPane.paneCommandHistoryIndex < currentPane.paneCommandHistory.size() - 1) {
            currentPane.paneCommandHistoryIndex++;
        }
        // Reset to initial state if at the end of history
        else {
            currentPane.paneCommandHistoryIndex = -1;
        }
    }
    
    // Update input based on selected history item
    if (currentPane.paneCommandHistoryIndex != -1) {
        // Construct full command with path
        std::string fullCommand = currentPath +
            currentPane.paneCommandHistory[currentPane.paneCommandHistoryIndex];
        
        // Update pane input
        currentPane.currentInput = fullCommand;
        currentPane.inputText.setString(fullCommand);
        
        // Set cursor position to end of command
        currentPane.cursorPosition =
        currentPane.paneCommandHistory[currentPane.paneCommandHistoryIndex].length();
        
        guiLogger.log("[DEBUG](ClientGUI::navigatePaneCommandHistory) Pane command selected: '" +
                      currentPane.paneCommandHistory[currentPane.paneCommandHistoryIndex] +
                      "'. Index: " + std::to_string(currentPane.paneCommandHistoryIndex));
    } else {
        // Reset to initial state
        currentPane.currentInput = currentPath;
        currentPane.inputText.setString(currentPath);
        currentPane.cursorPosition = 0;
        
        guiLogger.log("[DEBUG](ClientGUI::navigatePaneCommandHistory) Pane command history reset.");
    }
}

std::vector<std::string> ClientGUI::splitFileContent(const std::string& content) {
    guiLogger.log("[DEBUG](ClientGUI::splitFileContent) Splitting file content. Total length: " +
                  std::to_string(content.length()) + " bytes.");
    
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;
    
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }
    
    // Ensure at least one line
    if (lines.empty()) {
        guiLogger.log("[WARN](ClientGUI::splitFileContent) No lines found. Creating empty line.");
        lines.push_back("");
    }
    
    guiLogger.log("[DEBUG](ClientGUI::splitFileContent) Split result: " + std::to_string(lines.size()) + " lines.");
    return lines;
}

void ClientGUI::processInput(sf::Event event) {
    try {
        
        if (currentMode == editorMode::EDITTING) {
            guiLogger.log("[DEBUG](ClientGUI::processInput) Currently in nano editor mode, processing nano input.");
            processNanoInput(event);
            return;
        }
        if (!panes.empty()) {
            // Work with the current pane
            Pane& currentPane = panes[currentPaneIndex];
            
            // Process input for the current pane
            processPaneInput(event, currentPane);
        } else
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode < 128) {
                char inputChar = static_cast<char>(event.text.unicode);
                std::string currentInput = this -> inputText.getString();
                std::string currentPath = this -> backend.GetPath() + "> ";
                
                // Handle Enter
                if (inputChar == '\r' || inputChar == '\n') {
                    std::string command;
                    
                    if (currentInput == currentPath) {
                        addLineToTerminal(currentPath);
                        this -> inputText.setString(currentPath);
                        guiLogger.log("[DEBUG](ClientGUI::ProcessInput) Empty input, added new line.");
                        return;
                    }
                    
                    if (currentInput.length() > currentPath.length()) {
                        command = currentInput.substr(currentPath.length());
                        this -> commandHistory.push_back(command);
                    }
                    
                    if (!command.empty()) {
                        try {
                            addLineToTerminal(currentInput);
                            std::string response = this -> backend.sendCommand(command);
                            
                            // check if the command is cd
                            if (command.substr(0, 2) == "cd") {
                                if (response.find("Invalid directory") == std::string::npos &&
                                    response.find("Error") == std::string::npos) {
                                    std::string newPath = response.substr(response.find_last_of('\n') + 1);
                                    this -> backend.SetPath(newPath);
                                    addLineToTerminal("Changed directory to: " + newPath);
                                } else {
                                    addLineToTerminal(response);
                                }
                            } else if (command.substr(0, 4) == "nano") { // check if the command is nano
                                
                                std::string fullPath = command.substr(5);
                                std::string fileName = std::filesystem::path(fullPath).filename().string();
                                std::string fileContent = this -> backend.sendCommand(command);
                                if(fileContent.substr(0,4) == "nano"){
                                    fileContent.substr(5);
                                }
                                guiLogger.log("[DEBUG](ClientGUI::ProcessInput) Server response for nano: " + fileContent);
                                
                                if (fileContent.find("Error") == std::string::npos) {
                                    bool fileMode = (fileContent == "NEW_FILE");
                                    // Enter nano editor mode
                                    this -> currentMode = editorMode::EDITTING;
                                    if(!fileMode){
                                        enterNanoEditorMode(fileContent, fileName);
                                    }
                                    else {
                                        enterNanoEditorMode("", fileName);
                                    }
                                    return;
                                } else {
                                    addLineToTerminal(fileContent);
                                }
                            } else // check if the command is exit
                                if (command == "exit") {
                                    addLineToTerminal(response);
                                    this -> window.clear(sf::Color::Black);
                                    this -> window.draw(this -> outputText);
                                    this -> window.draw(this -> scrollBar);
                                    this -> window.display();
                                    sf::sleep(sf::milliseconds(700));
                                    this -> window.close();
                                    return;
                                } else if (command == "clear") {
                                    this -> terminalLines.clear();
                                    updateTerminalDisplay();
                                    updateScrollBar();
                                    
                                    std::string newCurrentPath = this -> backend.GetPath() + "> ";
                                    this -> inputText.setString(newCurrentPath);
                                    this -> cursorPosition = 0.0f;
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
                            std::string newCurrentPath = this -> backend.GetPath() + "> ";
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
        guiLogger.log("[ERROR](ClientGUI::processInput) Exception: " + std::string(e.what()));
        addLineToTerminal("Error: " + std::string(e.what()));
    }
    catch (...) {
        guiLogger.log("[ERROR](ClientGUI::processInput) Unknown exception occurred.");
        addLineToTerminal("An unknown error occurred.");
    }
}

void ClientGUI::addLineToTerminal(const std::string &line) {
    std::string trimmedLine = line;
    trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);
    
    if (!trimmedLine.empty()) {
        float maxWidth = this -> window.getSize().x - 20;
        
        sf::Text tempText;
        tempText.setFont(this -> font);
        tempText.setCharacterSize(this -> inputText.getCharacterSize());
        
        // identify spaces/tabs accurately
        size_t initialWhitespaceLength = trimmedLine.find_first_not_of(" \t");
        std::string initialWhitespace = trimmedLine.substr(0, initialWhitespaceLength);
        std::string remainingText = trimmedLine.substr(initialWhitespaceLength);
        
        std::istringstream words(remainingText);
        std::string word;
        std::string wrappedLine = initialWhitespace;
        
        while (words >> word) {
            std::string testLine = wrappedLine + word + " ";
            tempText.setString(testLine);
            
            // Check if line exceeds maximum width
            if (tempText.getGlobalBounds().width > maxWidth) {
                this -> terminalLines.push_back(wrappedLine);
                wrappedLine = initialWhitespace + word + " ";
            } else {
                wrappedLine += word + " ";
            }
        }
        
        // Add last line
        if (!wrappedLine.empty()) {
            this -> terminalLines.push_back(wrappedLine);
        }
        
        // Limit terminal history
        if (this -> terminalLines.size() > MAX_HISTORY) {
            this -> terminalLines.erase(
                                        this -> terminalLines.begin(),
                                        this -> terminalLines.begin() + (this -> terminalLines.size() - MAX_HISTORY)
                                        );
        }
        
        // Reset scroll position
        this -> scrollPosition = 0;
        
        // Update display
        updateTerminalDisplay();
        updateScrollBar();
        
        this -> guiLogger.log("[DEBUG] Terminal line added. Total lines: " +
                              std::to_string(terminalLines.size()));
    }
}

void ClientGUI::run() {
    // Smooth trackpad movement accumulator
    float trackpadScrollAccumulator = 0.0f;
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return;
            }
            
            if(event.type == sf::Event::KeyPressed) {
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                    switch(event.key.code) {
                        case sf::Keyboard::H: // horizontal
                            createNewPane(SplitType::HORIZONTAL);
                            break;
                        case sf::Keyboard::V: // vertical
                            createNewPane(SplitType::VERTICAL);
                            break;
                        case sf::Keyboard::Tab: // horizontal
                        {
                            if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)){
                                switchPane(-1); // move backwards
                            }
                            else switchPane(1);
                        }
                            break;
                        case sf::Keyboard::W:
                            closeCurrentPane();
                            break;
                        default:
                            break;
                    }
                }
            }
            
            // Mode-specific processing
            if (currentMode == editorMode::EDITTING) {
                processNanoInput(event);
            } else if(currentMode == editorMode::NORMAL) {
                // Normal processing
                if(this -> panes.empty()){
                    if (event.type == sf::Event::TextEntered) {
                        processInput(event);
                    }
                    
                    if (event.type == sf::Event::KeyPressed) {
                        if (currentMode != editorMode::EDITTING) {
                            handleSpecialInput(event);
                        }
                    }
                } else {
                    Pane& currentPane = panes[currentPaneIndex];
                    if (event.type == sf::Event::TextEntered) {
                        processPaneInput(event, currentPane);
                    }
                    
                    if (event.type == sf::Event::KeyPressed) {
                        if (currentMode != editorMode::EDITTING) {
                            handlePaneSpecialInput(event, currentPane);
                        }
                    }
                }
            }
            
            // Scroll handling
            if (event.type == sf::Event::MouseWheelScrolled) {
                trackpadScrollAccumulator += event.mouseWheelScroll.delta;
                if (std::abs(trackpadScrollAccumulator) >= 1.0f) {
                    int scrollLines = static_cast<int>(trackpadScrollAccumulator);
                    if (scrollLines > 0) { // Positive means scroll down
                        scrollUp(1);
                    }
                    else if (scrollLines < 0) { // Negative means scroll up
                        scrollDown(1);
                    }
                }
                trackpadScrollAccumulator = 0.0f;
            }
        }
        
        // Cursor blinking logic for normal mode
        if (this -> currentMode == editorMode::NORMAL) {
            if (this -> cursorBlinkClock.getElapsedTime() >= this -> cursorBlinkInterval) {
                this -> cursorVisible = !this -> cursorVisible;
                this -> cursorBlinkClock.restart();
                
                this -> cursor.setFillColor(this -> cursorVisible ?
                                            sf::Color::White : sf::Color::Transparent);
            }
        }
        
        // for unified drawing
        window.clear(sf::Color::Black);
        
        // Update based on current mode
        if (currentMode == editorMode::EDITTING) {
            // Nano editor mode updates
            window.clear(sf::Color::Black);
            refreshNanoDisplay();
            window.display();
        } else {
            // Normal mode updates
            if (panes.empty()) {
                // Mod fără pane-uri
                updateCursor();
                updateTerminalDisplay();
                updateScrollBar();
                
                if (!outputText.getString().isEmpty()) {
                    window.draw(outputText);
                }
                
                window.draw(inputText);
                window.draw(cursor);
                
                if (scrollBar.getSize().y > 0) {
                    window.draw(scrollBar);
                }
            } else {
                // Pane mods
                for (auto& pane : panes) {
                    // view for every pane
                    sf::View paneView(sf::FloatRect(0, 0, window.getSize().x, window.getSize().y));
                    window.setView(paneView);
                    
                    pane.outputText.setCharacterSize(16);
                    pane.inputText.setCharacterSize(16);
                    
                    window.draw(pane.outputText);
                    window.draw(pane.inputText);
                    window.draw(pane.cursor);
                    
                    // Save the curent state
                    auto tempTerminalLines = terminalLines;
                    auto tempCurrentPath = backend.GetPath();
                    auto tempInputString = inputText.getString();
                    auto tempScrollPosition = scrollPosition;
                    
                    // Restore the pane state
                    terminalLines = pane.terminalLines;
                    backend.SetPath(pane.currentPath);
                    inputText.setString(pane.currentInput);
                    scrollPosition = pane.scrollPosition;
                    
                    // Divider line
                    sf::RectangleShape divider;
                    divider.setFillColor(sf::Color(100, 100, 100, 200)); // Semi-transparent Gray
                    divider.setSize(sf::Vector2f(window.getSize().x, 2));
                    
                    if (panes.size() > 1) {
                        if (pane.splitType == SplitType::HORIZONTAL) {
                            divider.setPosition(0, window.getSize().y / 2 - 1);
                        } else {
                            divider.setPosition(window.getSize().x / 2 - 1, 0);
                            divider.setSize(sf::Vector2f(2, window.getSize().y));
                        }
                        window.draw(divider);
                    }
                    
                    // Update the pane content on the screen
                    updatePaneCursor(pane);
                    updatePaneTerminalDisplay(pane);
                    updatePaneScrollBar(pane);
                    
                   
                  
                    
                    // Restore the previous state
                    terminalLines = tempTerminalLines;
                    backend.SetPath(tempCurrentPath);
                    inputText.setString(tempInputString);
                    scrollPosition = tempScrollPosition;
                }
            }
            
            // Restore the original view
            window.setView(window.getDefaultView());
            
            window.display();
        }
    }
}

}
