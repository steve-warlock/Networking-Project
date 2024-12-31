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
        
        this -> editorLines.push_back("File saved: " + this -> currentEditingFile);
        
    }
    catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::saveNanoFile) Save failed: " +
                      std::string(e.what()));
        
        this -> editorLines.push_back("File saved: " + this -> currentEditingFile);
    }
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
    size_t renderedLines = 0;
    for (size_t i = nanoCursor.scrollOffset;
         i < std::min(nanoCursor.scrollOffset + maxVisibleLines, this->editorLines.size());
         ++i) {
        // Add ">" if line is too long
        std::string displayLine = this->editorLines[i];
        bool lineExceedsWidth = false;
        
        contentText.setString(displayLine);
        
        if (contentText.getLocalBounds().width > window.getSize().x - 30) {
            size_t truncateIndex = displayLine.length();
            while (truncateIndex > 0) {
                contentText.setString(displayLine.substr(0, truncateIndex + 1) + ">");
                if (contentText.getLocalBounds().width <= window.getSize().x - 30) {
                    break;
                }
                truncateIndex--;
            }
            
            displayLine = displayLine.substr(0, truncateIndex + 3) + ">";
            lineExceedsWidth = true;
        }
        
        if (lineExceedsWidth && i == nanoCursor.line) {
            displayLine = this->editorLines[i];
        }
        
        contentText.setString(displayLine);
        contentText.setPosition(10, yPosition);
        
        // Highlight current line
        if (i == nanoCursor.line) {
            sf::RectangleShape lineHighlight;
            lineHighlight.setSize(sf::Vector2f(window.getSize().x, 25));
            lineHighlight.setPosition(0, yPosition);
            lineHighlight.setFillColor(sf::Color(50, 50, 50, 100));
            renderTexture.draw(lineHighlight);
        }
        
        renderTexture.draw(contentText);
        yPosition += 25;
        renderedLines++;
    }
    
    // Log rendering details
    guiLogger.log("[DEBUG](ClientGUI::refreshNanoDisplay) Rendered lines: " +
                  std::to_string(renderedLines) +
                  ", Total lines: " + std::to_string(this->editorLines.size()));
    
    // Footer text
    sf::Text footerText;
    footerText.setFont(this->font);
    footerText.setCharacterSize(16);
    footerText.setFillColor(sf::Color::Green);
    footerText.setString("^O Save   ^X Exit");
    footerText.setPosition(10, this -> window.getSize().y - 40);
    renderTexture.draw(footerText);
    
    // get sprite textures
    sf::Sprite sprite(renderTexture.getTexture());
    
    // Draw cursor
    renderTexture.draw(cursor);
    
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
    this -> nanoCursor = {0, 0};
    
    // Set current mode
    this->currentMode = editorMode::EDITTING;
    
    // hide terminal cursor
    this -> cursor.setSize(sf::Vector2f(0, 0));
    
    // Save filename
    this->currentEditingFile = fileName;
    
    // Reset editor lines
    this->editorLines.clear();
    
    // Split content into lines
    if(!fileContent.empty()){
        std::istringstream contentStream(fileContent);
        std::string line;
        while (std::getline(contentStream, line)) {
            this->editorLines.push_back(line);
        }
    }
    
    // Add empty line if no content
    if (this->editorLines.empty()) {
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
                                    // Enter nano editor mode
                                    this -> currentMode = editorMode::EDITTING;
                                    if(!fileContent.empty()){
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
            
            // Mode-specific processing
            if (currentMode == editorMode::EDITTING) {
                processNanoInput(event);
            } else if(currentMode == editorMode::NORMAL) {
                // Normal processing
                if (event.type == sf::Event::TextEntered) {
                    processInput(event);
                }
                
                if (event.type == sf::Event::KeyPressed) {
                    if (currentMode != editorMode::EDITTING) {
                        handleSpecialInput(event);
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
            window.clear(sf::Color::Black);
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
            
            window.display();
        }
    }
}

}
