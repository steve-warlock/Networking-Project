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

ClientGUI::~ClientGUI() {
    try {
        // Log destructor entry
        guiLogger.log("[DEBUG](ClientGUI::~ClientGUI) Starting destructor.");
        
        // Close the window if it's still open
        if (window.isOpen()) {
            window.close();
            guiLogger.log("[INFO](ClientGUI::~ClientGUI) Window closed.");
        }
        
        // Clean up panes
        for (auto& pane : panes) {
            // Explicitly close backend connections
            if (pane.backend) {
                guiLogger.log("[DEBUG](ClientGUI::~ClientGUI) Closing pane backend.");
                // The unique_ptr will automatically delete the backend
                pane.backend.reset();
            }
        }
        
        // Clear panes vector
        panes.clear();
        
        // Log successful cleanup
        guiLogger.log("[DEBUG](ClientGUI::~ClientGUI) Destructor completed successfully.");
    }
    catch (const std::exception& e) {
        // Log any errors during destruction
        try {
            guiLogger.log("[ERROR](ClientGUI::~ClientGUI) Exception during destruction: " +
                          std::string(e.what()));
        }
        catch (...) {
            // Fallback error logging in case logger fails
            std::cerr << "Critical error in ClientGUI destructor" << std::endl;
        }
    }
    catch (...) {
        // Catch any unexpected errors
        try {
            guiLogger.log("[CRITICAL](ClientGUI::~ClientGUI) Unknown error during destruction.");
        }
        catch (...) {
            std::cerr << "Critical unknown error in ClientGUI destructor" << std::endl;
        }
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
            this -> nanoCursor = {0, 0, 0};
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
    this -> editorLines.clear();
    this -> currentEditingFile = "";
    
    // Restore terminal state
    std::string currentPath = this -> backend.GetPath() + "> ";
    this->inputText.setString(currentPath);
    
    guiLogger.log("[DEBUG](ClientGUI::exitNanoEditor) Nano editor state reset.");
}

// pane functions
void ClientGUI::createNewPane(SplitType splitType) {
    // Log entry point and current state
    guiLogger.log("[DEBUG](ClientGUI::createNewPane) Entering method.");
    
    // Pane limit check
    if (panes.size() >= 4) {
        guiLogger.log("[WARN](ClientGUI::createNewPane) Maximum pane limit (4) reached.");
        addLineToTerminal("Maximum panes (4) reached!");
        return;
    }
    
    try {
        // First pane creation logic
        if (panes.empty()) {
            Pane initialPane;
            initialPane.splitType = splitType;
            
            // Backend cloning
            initialPane.backend = backend.clone();
            
            // Initialize current path
            std::string path = backend.GetPath();
            initialPane.backend -> SetPath(path);
            initialPane.currentInput = initialPane.backend -> GetPath() + "> ";
            initialPane.inputText.setString(initialPane.currentInput);
            
            // Text configuration
            initialPane.inputText.setFont(font);
            initialPane.outputText.setFont(font);
            initialPane.inputText.setCharacterSize(16);
            initialPane.outputText.setCharacterSize(16);
            initialPane.inputText.setFillColor(sf::Color::White);
            initialPane.outputText.setFillColor(sf::Color::White);
            initialPane.outputText.setString("");
            
            // Initialize cursor
            initialPane.cursor.setSize(sf::Vector2f(2, 16));
            initialPane.cursor.setFillColor(sf::Color::White);
            
            
            panes.push_back(std::move(initialPane));
            
            initializePaneCursor(panes.back());
            updatePaneTerminalDisplay(panes.back());
        }
        
        // Subsequent pane creation
        Pane newPane;
        newPane.splitType = splitType;
        
        // Backend creation
        newPane.backend = std::make_unique<backend::ClientBackend>("127.0.0.1", 8080);
        
        // Initialize current path
        std::string path = current_path().string();
        newPane.backend -> SetPath(path);
        newPane.currentInput = newPane.backend -> GetPath() + "> ";
        newPane.inputText.setString(newPane.currentInput);
        // Text configuration
        newPane.inputText.setFont(font);
        newPane.outputText.setFont(font);
        newPane.inputText.setCharacterSize(16);
        newPane.outputText.setCharacterSize(16);
        newPane.inputText.setFillColor(sf::Color::White);
        newPane.outputText.setFillColor(sf::Color::White);
        newPane.inputText.setPosition(
                                      newPane.bounds.left + 10,  // Relative to pane's left bound
                                      newPane.bounds.top + 50    // With a consistent vertical offset
                                      );
        newPane.outputText.setString("");
        // Initialize cursor
        newPane.cursor.setSize(sf::Vector2f(2, 16));
        newPane.cursor.setFillColor(sf::Color::White);
        newPane.cursor.setPosition(
                                   newPane.inputText.getPosition().x,  // Aligned with input text
                                   newPane.inputText.getPosition().y + 2  // Slight vertical offset
                                   );
        
        panes.push_back(std::move(newPane));
        
        // Update current pane index
        currentPaneIndex = panes.size() - 1;
        
        // Force update pane bounds
        updatePaneBounds();
        
        // initailize cursor
        initializePaneCursor(panes[currentPaneIndex]);
        
        // Update terminal display for the new pane
        updatePaneTerminalDisplay(panes[currentPaneIndex]);
        
        // Log pane creation summary
        guiLogger.log("[DEBUG](ClientGUI::createNewPane) New pane created successfully");
        guiLogger.log("[DEBUG](ClientGUI::createNewPane) Total panes now: " + std::to_string(panes.size()));
        guiLogger.log("[DEBUG](ClientGUI::createNewPane) Current pane index: " + std::to_string(currentPaneIndex));
        
        // Force render
        renderPanes();
    }
    catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::createNewPane) Pane creation failed: " + std::string(e.what()));
        addLineToTerminal("Pane creation error: " + std::string(e.what()));
    }
}

void ClientGUI::initializePaneCursor(Pane& pane) {
    // create cursor line
    pane.cursor.setSize(sf::Vector2f(2, pane.inputText.getCharacterSize()));
    pane.cursor.setFillColor(sf::Color::White);
    
    // position cursor after shell path
    std::string currentPath = pane.backend -> GetPath() + "> ";
    const sf::Font* font = pane.inputText.getFont();
    
    // offset for cursor
    float pathOffset = 0;
    for (char c : currentPath) {
        pathOffset += font->getGlyph(c, pane.inputText.getCharacterSize(), false).advance;
    }
    
    // position cursor after shell path
    pane.cursor.setPosition(
                            pane.bounds.left + 10 + pathOffset, // 10 for padding
                            pane.inputText.getPosition().y + pane.bounds.top + 2
                            );
    
    // initiate the cursor position
    pane.cursorPosition = 0.0f;
    
    guiLogger.log("[DEBUG](ClientGUI::initializePaneCursor) Pane cursor initialized successfully.");
}

void ClientGUI::updatePaneBounds() {
    // Comprehensive logging and diagnostic information
    guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Starting pane bounds update");
    
    // Get window dimensions
    sf::Vector2u windowSize = window.getSize();
    size_t paneCount = panes.size();
    
    // Log critical information
    guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Window dimensions: " +
                  std::to_string(windowSize.x) + "x" +
                  std::to_string(windowSize.y));
    guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Total panes: " + std::to_string(paneCount));
    
    // Validate pane count
    if (paneCount == 0) {
        guiLogger.log("[WARN](ClientGUI::updatePaneBounds) No panes to update - skipping bounds calculation");
        return;
    }
    
    // Detailed pane information logging
    for (size_t i = 0; i < paneCount; ++i) {
        guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Pane " + std::to_string(i) + " split type: " +
                      (panes[i].splitType == SplitType::HORIZONTAL ? "HORIZONTAL" : "VERTICAL"));
    }
    
    // Comprehensive bounds calculation
    switch(paneCount) {
        case 1: {
            // Single pane fills entire window
            panes[0].bounds = sf::FloatRect(
                                            0, 0,                    // x, y position
                                            windowSize.x,            // width
                                            windowSize.y             // height
                                            );
            guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Single pane set to full window");
            break;
        }
            
        case 2: {
            // Two pane scenarios
            if (panes[0].splitType == SplitType::HORIZONTAL) {
                // Horizontal split (top and bottom)
                panes[0].bounds = sf::FloatRect(
                                                0, 0,                // x, y position
                                                windowSize.x,        // width
                                                windowSize.y / 2     // height
                                                );
                panes[1].bounds = sf::FloatRect(
                                                0, windowSize.y / 2, // x, y position
                                                windowSize.x,        // width
                                                windowSize.y / 2     // height
                                                );
                guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) 2 panes - Horizontal split");
            } else {
                // Vertical split (left and right)
                panes[0].bounds = sf::FloatRect(
                                                0, 0,                // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y         // height
                                                );
                panes[1].bounds = sf::FloatRect(
                                                windowSize.x / 2, 0, // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y         // height
                                                );
                guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) 2 panes - Vertical split");
            }
            break;
        }
            
        case 3: {
            // Three pane scenarios
            if (panes[0].splitType == SplitType::HORIZONTAL) {
                // Top full width, bottom split
                panes[0].bounds = sf::FloatRect(
                                                0, 0,                // x, y position
                                                windowSize.x,        // width
                                                windowSize.y / 2     // height
                                                );
                panes[1].bounds = sf::FloatRect(
                                                0, windowSize.y / 2, // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y / 2     // height
                                                );
                panes[2].bounds = sf::FloatRect(
                                                windowSize.x / 2, windowSize.y / 2, // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y / 2     // height
                                                );
                guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) 3 panes - Horizontal primary split");
            } else {
                // Left full height, right split
                panes[0].bounds = sf::FloatRect(
                                                0, 0,                // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y         // height
                                                );
                panes[1].bounds = sf::FloatRect(
                                                windowSize.x / 2, 0, // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y / 2     // height
                                                );
                panes[2].bounds = sf::FloatRect(
                                                windowSize.x / 2, windowSize.y / 2, // x, y position
                                                windowSize.x / 2,    // width
                                                windowSize.y / 2     // height
                                                );
                guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) 3 panes - Vertical primary split");
            }
            break;
        }
            
        case 4: {
            // Four panes - equal quadrants
            panes[0].bounds = sf::FloatRect(
                                            0, 0,                    // x, y position
                                            windowSize.x / 2,        // width
                                            windowSize.y / 2         // height
                                            );
            panes[1].bounds = sf::FloatRect(
                                            windowSize.x / 2, 0,     // x, y position
                                            windowSize.x / 2,        // width
                                            windowSize.y / 2         // height
                                            );
            panes[2].bounds = sf::FloatRect(
                                            0, windowSize.y / 2,     // x, y position
                                            windowSize.x / 2,        // width
                                            windowSize.y / 2         // height
                                            );
            panes[3].bounds = sf::FloatRect(
                                            windowSize.x / 2, windowSize.y / 2, // x, y position
                                            windowSize.x / 2,        // width
                                            windowSize.y / 2         // height
                                            );
            guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) 4 panes - Quadrant split");
            break;
        }
            
        default: {
            guiLogger.log("[ERROR](ClientGUI::updatePaneBounds) Unsupported number of panes: " + std::to_string(paneCount));
            break;
        }
    }
    
    // Log final bounds for verification
    for (size_t i = 0; i < paneCount; ++i) {
        guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Pane " + std::to_string(i) + " Final Bounds: " +
                      "Left: " + std::to_string(panes[i].bounds.left) +
                      ", Top: " + std::to_string(panes[i].bounds.top) +
                      ", Width: " + std::to_string(panes[i].bounds.width) +
                      ", Height: " + std::to_string(panes[i].bounds.height));
    }
    
    guiLogger.log("[DEBUG](ClientGUI::updatePaneBounds) Pane bounds update completed");
}

void ClientGUI::updatePaneTerminalDisplay(Pane& pane) {
    try {
        updatePaneBounds();
        
        const float TITLE_HEIGHT = 30.0f;
        const float CHAR_HEIGHT = 16.0f;
        const float LINE_SPACING = 4.0f;
        const float LINE_HEIGHT = CHAR_HEIGHT + LINE_SPACING;
        const float PADDING = 10.0f;
        const sf::Font* font = pane.inputText.getFont();
        
        // Calculate visible area
        float availableHeight = pane.bounds.height - TITLE_HEIGHT - (PADDING * 3) - LINE_HEIGHT;
        size_t maxVisibleLines = static_cast<size_t>(availableHeight / LINE_HEIGHT);
        
        // Calculate display range (show newest lines first)
        size_t startIndex;
        if (pane.terminalLines.size() > maxVisibleLines) {
            startIndex = pane.terminalLines.size() - maxVisibleLines - pane.scrollPosition;
        } else {
            startIndex = 0;
        }
        
        // Build display text from newest lines
        std::string displayText;
        for (size_t i = startIndex;
             i < pane.terminalLines.size() && i < startIndex + maxVisibleLines;
             ++i) {
            displayText += pane.terminalLines[i] + "\n";
        }
        if (!displayText.empty()) displayText.pop_back();
        
        // Draw title
        sf::Text titleText;
        titleText.setFont(*font);
        titleText.setCharacterSize(20);
        titleText.setFillColor(sf::Color::White);
        titleText.setString("Pane " + std::to_string(currentPaneIndex + 1));
        titleText.setPosition(pane.bounds.left + PADDING, pane.bounds.top + PADDING);
        
        // Position output text
        pane.outputText.setFont(*font);
        pane.outputText.setCharacterSize(CHAR_HEIGHT);
        pane.outputText.setFillColor(sf::Color::White);
        pane.outputText.setString(displayText);
        pane.outputText.setPosition(pane.bounds.left + PADDING,
                                    pane.bounds.top + TITLE_HEIGHT + PADDING);
        
        // Calculate input position below output
        float contentHeight = displayText.empty() ? 0 : pane.outputText.getLocalBounds().height;
        float inputY = pane.bounds.top + TITLE_HEIGHT + PADDING + contentHeight + LINE_SPACING;
        float maxInputY = pane.bounds.top + pane.bounds.height - LINE_HEIGHT - PADDING;
        inputY = std::min(inputY, maxInputY);
        
        // Position input text
        pane.inputText.setFont(*font);
        pane.inputText.setCharacterSize(CHAR_HEIGHT);
        pane.inputText.setFillColor(sf::Color::White);
        pane.inputText.setPosition(pane.bounds.left + PADDING, inputY);
        
        // Draw elements
        //        window.draw(titleText);
        //        window.draw(pane.outputText);
        //        window.draw(pane.inputText);
        
        // Update cursor and scrollbar
        updatePaneCursor(pane);
        updatePaneScrollBar(pane);
        
        guiLogger.log("[DEBUG] Display updated - Start: " + std::to_string(startIndex) +
                      ", Visible: " + std::to_string(maxVisibleLines) +
                      ", Total: " + std::to_string(pane.terminalLines.size()));
    } catch(const std::exception& e) {
        guiLogger.log("[ERROR] " + std::string(e.what()));
    }
}

void ClientGUI::addLineToPaneTerminal(Pane& currentPane, const std::string& line) {
    const size_t MAX_HISTORY = 1000;
    const float MAX_WIDTH = currentPane.bounds.width - 20;
    
    std::string trimmedLine = line;
    trimmedLine.erase(trimmedLine.find_last_not_of(" \t") + 1);
    if (trimmedLine.empty()) return;
    
    sf::Text tempText;
    tempText.setFont(font);
    tempText.setCharacterSize(16);
    tempText.setString(trimmedLine);
    
    bool isPrompt = trimmedLine.find(currentPane.backend->GetPath() + "> ") != std::string::npos;
    
    if (tempText.getLocalBounds().width > MAX_WIDTH && !isPrompt) {
        // Word wrap for long lines
        size_t whitespaceLen = trimmedLine.find_first_not_of(" \t");
        std::string indent = trimmedLine.substr(0, whitespaceLen);
        std::string text = trimmedLine.substr(whitespaceLen);
        
        std::istringstream words(text);
        std::string word, currentLine = indent;
        
        while (words >> word) {
            std::string testLine = currentLine + word + " ";
            tempText.setString(testLine);
            
            if (tempText.getLocalBounds().width > MAX_WIDTH) {
                if (currentLine != indent) {
                    currentPane.terminalLines.push_back(currentLine);
                }
                currentLine = indent + word + " ";
            } else {
                currentLine += word + " ";
            }
        }
        
        if (currentLine != indent) {
            currentPane.terminalLines.push_back(currentLine);
        }
    } else {
        // Add single line directly
        currentPane.terminalLines.push_back(trimmedLine);
    }
    
    // Limit history
    if (currentPane.terminalLines.size() > MAX_HISTORY) {
        size_t excess = currentPane.terminalLines.size() - MAX_HISTORY;
        currentPane.terminalLines.erase(
                                        currentPane.terminalLines.begin(),
                                        currentPane.terminalLines.begin() + excess
                                        );
    }
    
    currentPane.scrollPosition = 0;
    updatePaneCursor(currentPane);
    updatePaneScrollBar(currentPane);
    updatePaneTerminalDisplay(currentPane);
    
    guiLogger.log("[DEBUG](ClientGUI::addLineToPaneTerminal) Added line: '" + trimmedLine +
                  "'. Total lines: " + std::to_string(currentPane.terminalLines.size()));
}

void ClientGUI::updatePaneScrollBar(Pane& pane) {
    const float SCROLLBAR_WIDTH = 8.0f;
    const float PADDING = 2.0f;
    const float TITLE_HEIGHT = 30.0f;
    const float INPUT_HEIGHT = 20.0f;
    
    // Calculate dimensions
    float bottomEdge = pane.bounds.top + pane.bounds.height;
    float visibleHeight = pane.bounds.height - TITLE_HEIGHT - INPUT_HEIGHT - (PADDING);
    float contentHeight = pane.terminalLines.size() * INPUT_HEIGHT;
    
    if (contentHeight > visibleHeight) {
        // Position track from bottom
        sf::RectangleShape scrollTrack;
        scrollTrack.setSize(sf::Vector2f(SCROLLBAR_WIDTH, visibleHeight));
        scrollTrack.setPosition(
                                pane.bounds.left + pane.bounds.width - SCROLLBAR_WIDTH - PADDING,
                                bottomEdge - INPUT_HEIGHT - PADDING - visibleHeight
                                );
        scrollTrack.setFillColor(sf::Color(50, 50, 50));
        
        // Calculate thumb position from bottom
        float ratio = visibleHeight / contentHeight;
        float thumbHeight = std::max(30.0f, visibleHeight * ratio);
        float maxScroll = std::max(0.0f, static_cast<float>(pane.terminalLines.size() - (visibleHeight / 16.0f)));
        float scrollPercent = maxScroll > 0 ? pane.scrollPosition / maxScroll : 0;
        float thumbY = scrollTrack.getPosition().y + ((visibleHeight - thumbHeight) * (1.0f - scrollPercent));
        
        // Position thumb
        sf::RectangleShape scrollThumb;
        scrollThumb.setSize(sf::Vector2f(SCROLLBAR_WIDTH, thumbHeight));
        scrollThumb.setPosition(scrollTrack.getPosition().x, thumbY);
        scrollThumb.setFillColor(sf::Color(150, 150, 150));
        
        window.draw(scrollTrack);
        window.draw(scrollThumb);
        
        guiLogger.log("[DEBUG](ClientGUI::updatePaneScrollBar) Bottom scrollbar - Y: " +
                      std::to_string(scrollTrack.getPosition().y) +
                      ", Height: " + std::to_string(visibleHeight));
    }
}

void ClientGUI::updatePaneCursor(Pane& pane) {
    // Determine the pane index
    size_t paneIndex = std::find_if(panes.begin(), panes.end(),
                                    [&pane](const Pane& p) { return &p == &pane; }) - panes.begin();
    
    // Log the start of cursor update for this specific pane
    guiLogger.log("[DEBUG](ClientGUI::updatePaneCursor) Updating cursor for Pane " +
                  std::to_string(paneIndex) +
                  ", Current Pane Index: " + std::to_string(currentPaneIndex));
    
    // Get current path with shell prompt
    std::string currentPath = pane.backend->GetPath() + "> ";
    const sf::Font* font = pane.inputText.getFont();
    
    // Get entire input string
    std::string fullInput = pane.inputText.getString();
    std::string currentInput = fullInput.substr(currentPath.length());
    
    // Ensure cursor position does not exceed input length
    float maxCursorPosition = static_cast<float>(currentInput.length());
    pane.cursorPosition = std::min(std::max(0.0f, pane.cursorPosition), maxCursorPosition);
    
    // Temporary text to calculate cursor position
    sf::Text tempText;
    tempText.setFont(*font);
    tempText.setCharacterSize(pane.inputText.getCharacterSize());
    tempText.setString(currentPath + currentInput);
    
    float maxWidth = pane.bounds.width - 20; // 20px padding
    float textWidth = tempText.getGlobalBounds().width;
    static float scrollOffset = 0.0f;
    
    if (textWidth > maxWidth) {
        float cursorOffset = tempText.findCharacterPos(currentPath.length() + static_cast<int>(pane.cursorPosition)).x;
        float targetOffset = cursorOffset - (maxWidth * 0.8f);
        scrollOffset = std::max(0.0f, targetOffset);
    } else {
        scrollOffset = 0.0f;
    }
    
    // Calculate cursor offset
    float cursorOffset = tempText.findCharacterPos(currentPath.length() + static_cast<int>(pane.cursorPosition)).x;
    
    // Determine vertical position
    float verticalPosition = pane.inputText.getPosition().y;
    
    pane.inputText.setPosition(pane.bounds.left + 10 - scrollOffset, verticalPosition);
    
    // Update cursor position with bounds checking
    float finalCursorX = std::min(std::max(pane.bounds.left + 10 + cursorOffset, pane.bounds.left + 10), pane.bounds.left + pane.bounds.width);
    // Detailed logging of cursor calculation
    guiLogger.log("[DEBUG](ClientGUI::updatePaneCursor) Pane " + std::to_string(paneIndex) +
                  " Cursor Details: CurrentPath: '" + currentPath + "' CurrentInput: '" + currentInput + "' CursorOffset: " + std::to_string(cursorOffset) +
                  " PaneBounds Left: " + std::to_string(pane.bounds.left) +
                  " InputText Y: " + std::to_string(pane.inputText.getPosition().y));
    
    // Update cursor position relative to pane bounds
    pane.cursor.setPosition(
                            finalCursorX,  // Add 10 for padding
                            verticalPosition + 2
                            );
    
    // Cursor visibility logic
    if (paneIndex == currentPaneIndex) {
        // Blinking cursor for active pane
        static sf::Clock blinkClock;
        static bool cursorVisible = true;
        
        if (blinkClock.getElapsedTime().asSeconds() >= 0.5f) {
            cursorVisible = !cursorVisible;
            blinkClock.restart();
        }
        
        pane.cursor.setFillColor(cursorVisible ? sf::Color::White : sf::Color::Transparent);
        
        // Log cursor visibility state
        guiLogger.log("[DEBUG](ClientGUI::updatePaneCursor) Pane " + std::to_string(paneIndex) +
                      " Cursor Visibility: " + (cursorVisible ? "Visible" : "Hidden"));
    } else {
        pane.cursor.setFillColor(sf::Color::Transparent);
        
        // Log inactive pane cursor state
        guiLogger.log("[DEBUG](ClientGUI::updatePaneCursor) Pane " + std::to_string(paneIndex) +
                      " Cursor: Hidden (Inactive Pane)");
    }
    
    // Log final cursor position
    guiLogger.log("[DEBUG](ClientGUI::updatePaneCursor) Pane " + std::to_string(paneIndex) +
                  " Final Cursor Position: X=" + std::to_string(pane.cursor.getPosition().x) +
                  " Y=" + std::to_string(pane.cursor.getPosition().y));
}

void ClientGUI::renderPanes() {
    // Comprehensive logging and debugging
    guiLogger.log("[DEBUG](ClientGUI::renderPanes) Starting pane rendering");
    
    // Ensure we have panes to render
    if (panes.empty()) {
        guiLogger.log("[WARN](ClientGUI::renderPanes) No panes to render");
        return;
    }
    
    // Clear the window with a distinct background color
    window.clear(sf::Color(30, 30, 30)); // Dark gray background for visibility
    
    // Force update pane bounds
    updatePaneBounds();
    
    // Log total panes
    guiLogger.log("[DEBUG](ClientGUI::renderPanes) Rendering " + std::to_string(panes.size()) + " panes");
    
    // Render each pane with distinct visual characteristics
    for (size_t i = 0; i < panes.size(); ++i) {
        Pane& pane = panes[i];
        bool isActivePane = (i == currentPaneIndex);
        
        // Detailed pane bounds logging
        guiLogger.log("[DEBUG](ClientGUI::renderPanes) Pane " + std::to_string(i) + " Bounds: Left: " + std::to_string(pane.bounds.left) +
                      ", Top: " + std::to_string(pane.bounds.top) +
                      ", Width: " + std::to_string(pane.bounds.width) +
                      ", Height: " + std::to_string(pane.bounds.height));
        
        // Create a rectangle shape for the pane with distinct visual properties
        sf::RectangleShape paneBackground;
        paneBackground.setPosition(pane.bounds.left, pane.bounds.top);
        paneBackground.setSize(sf::Vector2f(pane.bounds.width, pane.bounds.height));
        
        // Use distinct colors for each pane
        sf::Color paneColors[] = {
            sf::Color(50, 0, 0, 150),    // Dark Red
            sf::Color(0, 50, 0, 150),    // Dark Green
            sf::Color(0, 0, 50, 150),    // Dark Blue
            sf::Color(50, 50, 0, 150)    // Dark Yellow
        };
        paneBackground.setFillColor(paneColors[i % 4]);
        
        // Add a border to make panes more distinct
        paneBackground.setOutlineThickness(3);
        paneBackground.setOutlineColor(
                                       isActivePane ? sf::Color::Green : sf::Color(100, 100, 100, 200)
                                       );
        
        // Draw the pane background
        window.draw(paneBackground);
        
        // Add a text label to each pane for debugging
        sf::Text paneLabel;
        paneLabel.setFont(font);
        paneLabel.setCharacterSize(20);
        paneLabel.setString("Pane " + std::to_string(i + 1));
        paneLabel.setFillColor(sf::Color::White);
        paneLabel.setPosition(
                              pane.bounds.left + 10,
                              pane.bounds.top + 10
                              );
        window.draw(paneLabel);
        
        // Render input and output text for each pane
        try {
            
            updatePaneTerminalDisplay(pane);
            // Draw input and output text
            window.draw(pane.inputText);
            window.draw(pane.outputText);
            
            // Cursor and scrollbar handling
            updatePaneCursor(pane);
            
            if (isActivePane) {
                static sf::Clock blinkClock;
                static bool cursorVisible = true;
                
                if (blinkClock.getElapsedTime().asSeconds() >= 0.5f) {
                    cursorVisible = !cursorVisible;
                    blinkClock.restart();
                }
                pane.cursor.setFillColor(cursorVisible ? sf::Color::White : sf::Color::Transparent);
                window.draw(pane.cursor);
            } else {
                // Dimmed cursor for inactive panes
                pane.cursor.setFillColor(sf::Color(100, 100, 100, 50));
                window.draw(pane.cursor);
            }
        }
        catch (const std::exception& e) {
            guiLogger.log("[ERROR](ClientGUI::renderPanes) Failed to render pane text: " +
                          std::string(e.what()));
        }
        
    }
    
    // Display the rendered frame
    window.display();
    
    guiLogger.log("[DEBUG](ClientGUI::renderPanes) Pane rendering completed");
}

void ClientGUI::switchPane(int direction) {
    if (panes.empty()) return;
    
    Pane& currentPane = panes[currentPaneIndex];
    
    currentPane.terminalLines = this->terminalLines;
    std::string path = this -> backend.GetPath();
    currentPane.backend -> SetPath(path);
    currentPane.currentInput = this->inputText.getString();
    currentPane.scrollPosition = this->scrollPosition;
    
    currentPaneIndex = (currentPaneIndex + direction + panes.size()) % panes.size();
    
    Pane& newPane = panes[currentPaneIndex];
    this->terminalLines = newPane.terminalLines;
    path = newPane.backend -> GetPath();
    this->backend.SetPath(path);
    this->inputText.setString(newPane.currentInput);
    this->scrollPosition = newPane.scrollPosition;
    
    // Update display based on restored state
    initializePaneCursor(newPane);
    updatePaneScrollBar(newPane);
    updatePaneTerminalDisplay(newPane);
    
    // Log the pane switch
    guiLogger.log("[DEBUG](ClientGUI::switchPane) Switched to pane " +
                  std::to_string(currentPaneIndex) +
                  ". Current path: " + newPane.backend -> GetPath());
    
}

void ClientGUI::closeCurrentPane() {
    if (panes.size() <= 1) {
        
        this->terminalLines.clear();
        std::string terminal_path = current_path().string();
        this->backend.SetPath(terminal_path);
        this->inputText.setString(backend.GetPath() + "> ");
        this->scrollPosition = 0;
        
        panes.clear();
        currentPaneIndex = 0;
    } else {
        panes.erase(panes.begin() + currentPaneIndex);
        
        updatePaneBounds();
        
        currentPaneIndex = std::min(currentPaneIndex, panes.size() - 1);
        
//        Pane& remainingPane = panes[currentPaneIndex];
//        this->terminalLines = remainingPane.terminalLines;
//        std::string path = remainingPane.backend -> GetPath();
//        this->backend.SetPath(path);
//        this->inputText.setString(remainingPane.currentInput);
//        this->scrollPosition = remainingPane.scrollPosition;
        
    }
}

void ClientGUI::scrollPaneUp(Pane& pane, int lines) {
    size_t maxVisibleLines = std::min(
                                      MAX_VISIBLE_LINES,
                                      static_cast<size_t>(pane.bounds.height / pane.inputText.getCharacterSize())
                                      );
    
    if (pane.terminalLines.size() > maxVisibleLines) {
        // Scroll up to see older lines
        pane.scrollPosition = std::min(
                                       pane.scrollPosition + lines,
                                       std::max(0, static_cast<int>(pane.terminalLines.size()) - static_cast<int>(maxVisibleLines))
                                       );
        
        updatePaneTerminalDisplay(pane);
        updatePaneScrollBar(pane);
        
        guiLogger.log("[DEBUG](ClientGUI::scrollPaneUp) Scrolled up " +
                      std::to_string(lines) + " lines. Position: " +
                      std::to_string(pane.scrollPosition));
    }
}

void ClientGUI::scrollPaneDown(Pane& pane, int lines) {
    if (pane.scrollPosition > 0) {
        // Scroll down to see newer lines
        pane.scrollPosition = std::max(0, pane.scrollPosition - lines);
        
        updatePaneTerminalDisplay(pane);
        updatePaneScrollBar(pane);
        
        guiLogger.log("[DEBUG](ClientGUI::scrollPaneDown) Scrolled down " +
                      std::to_string(lines) + " lines. Position: " +
                      std::to_string(pane.scrollPosition));
    }
}

void ClientGUI::handlePaneScrolling(sf::Event event, Pane& pane) {
    pane.scrollAccumulator += event.mouseWheelScroll.delta;
    
    guiLogger.log("[DEBUG](ClientGUI::handlePaneScrolling) Terminal lines count: " + std::to_string(pane.terminalLines.size()));
    if (!pane.terminalLines.empty()) {
        guiLogger.log("[DEBUG] First line: " + pane.terminalLines.front());
        guiLogger.log("[DEBUG] Last line: " + pane.terminalLines.back());
    }
    guiLogger.log("[DEBUG](ClientGUI::handlePaneScrolling) Current scroll position: " + std::to_string(pane.scrollPosition));
    
    if (std::abs(pane.scrollAccumulator) >= 1.0f) {
        int scrollLines = static_cast<int>(pane.scrollAccumulator);
        
        try {
            // Invert scroll direction to match normal mode
            if (scrollLines > 0) {
                scrollPaneUp(pane, 1);  // Show older lines (higher index)
            } else if (scrollLines < 0) {
                scrollPaneDown(pane, 1);    // Show newer lines (lower index)
            }
        } catch (const std::exception& e) {
            guiLogger.log("[ERROR](ClientGUI::handlePaneScrolling) Scroll failed: " + std::string(e.what()));
        }
        
        pane.scrollAccumulator = 0.0f;
    }
}

void ClientGUI::processPaneInput(sf::Event event, Pane& currentPane) {
    
    if (currentMode == editorMode::EDITTING) {
        guiLogger.log("[DEBUG](ClientGUI::processPaneInput) Currently in nano editor mode, processing nano input.");
        processNanoInput(event);
        return;
    }
    
    if (event.type == sf::Event::TextEntered) {
        if (event.text.unicode < 128) {
            char inputChar = static_cast<char>(event.text.unicode);
            std::string currentPath = currentPane.backend -> GetPath() + "> ";
            std::string& currentInput = currentPane.currentInput;
            
            // Handle Enter key (execute command)
            if (inputChar == '\r' || inputChar == '\n') {
                // Extract command from input
                std::string command;
                
                if(currentPath == currentInput) {
                    addLineToPaneTerminal(currentPane, currentPath);
                    currentPane.inputText.setString(currentPath);
                    guiLogger.log("[DEBUG](ClientGUI::ProcessPaneInput) Empty input, added new line.");
                    return;
                }
                
                if (currentInput.length() > currentPath.length()) {
                    command = currentInput.substr(currentPath.length());
                    currentPane.commandHistory.push_back(command);
                    currentPane.currentHistoryIndex = -1;
                }
                
                if (!command.empty()) {
                    try {
                        // Add command to terminal lines
                        addLineToPaneTerminal(currentPane, currentInput);
                        
                        // Send command to backend
                        std::string response = currentPane.backend->sendCommand(command);
                        
                        if (command.substr(0, 2) == "cd") {
                            std::string response = currentPane.backend->sendCommand(command);
                            if (response.find("Invalid directory") == std::string::npos &&
                                response.find("Error") == std::string::npos) {
                                std::string newPath = response.substr(response.find_last_of('\n') + 1);
                                currentPane.backend->SetPath(newPath);
                                addLineToPaneTerminal(currentPane, "Changed directory to: " + newPath);
                            } else {
                                addLineToPaneTerminal(currentPane, response);
                            }
                        }
                        else
                            if (command.substr(0, 4) == "nano") {
                                std::string fullPath = command.substr(5);
                                std::string fileName = std::filesystem::path(fullPath).filename().string();
                                std::string fileContent = currentPane.backend->sendCommand(command);
                                
                                if (fileContent.find("Error") == std::string::npos) {
                                    bool fileMode = (fileContent == "NEW_FILE");
                                    currentMode = editorMode::EDITTING;
                                    if (!fileMode) {
                                        enterNanoEditorMode(fileContent, fileName);
                                    } else {
                                        enterNanoEditorMode("", fileName);
                                    }
                                    
                                } else {
                                    addLineToPaneTerminal(currentPane, fileContent);
                                }
                            }
                        // Handle clear command
                            else if (command == "clear") {
                                currentPane.terminalLines.clear();
                                currentPane.scrollPosition = 0;
                                currentPane.currentInput = currentPane.backend->GetPath() + "> ";
                                currentPane.cursorPosition = currentPane.currentInput.length();
                                updatePaneTerminalDisplay(currentPane);
                            }
                        // Handle exit command
                            else if (command == "exit") {
                                std::string response = currentPane.backend->sendCommand(command);
                                addLineToPaneTerminal(currentPane, response);
                                sf::sleep(sf::milliseconds(700));
                                closeCurrentPane();
                            }
                            else {
                                // Process response
                                if(!response.empty()) {
                                    std::istringstream responseStream(response);
                                    std::string line;
                                    while (std::getline(responseStream, line)) {
                                        if (!line.empty()) {
                                            addLineToPaneTerminal(currentPane, line);
                                        }
                                    }
                                }
                            }
                        // Reset input
                        std::string newPrompt = currentPane.backend->GetPath() + "> ";
                        currentPane.currentInput = newPrompt;
                        currentPane.cursorPosition = newPrompt.length();
                        currentPane.inputText.setString(currentPane.currentInput);
                        updatePaneCursor(currentPane);
                    } catch(const std::exception& e) {
                        addLineToPaneTerminal(currentPane, "Error: " + std::string(e.what()));
                        currentPane.inputText.setString(currentPath);
                        currentPane.cursorPosition = 0.0f;
                        updatePaneCursor(currentPane);
                    }
                }
            }
            // Handle Backspace
            else if (inputChar == '\b') {
                size_t cursorPos = currentPath.length() + static_cast<size_t>(currentPane.cursorPosition);
                if (cursorPos > currentPath.length()) {
                    // Remove character before cursor
                    currentInput.erase(cursorPos - 1, 1);
                    currentPane.inputText.setString(currentPane.currentInput);
                    currentPane.cursorPosition--;
                    
                }
            }
            // Handle character input
            else if(inputChar >= 32) {
                size_t insertPos = currentPath.length() + static_cast<size_t>(currentPane.cursorPosition);
                // Limit input length
                if (currentInput.length() < currentPath.length() + 256) {
                    // Insert character at cursor position
                    currentInput.insert(insertPos, 1, inputChar);
                    currentPane.inputText.setString(currentInput);
                    currentPane.cursorPosition++;
                    
                }
            }
            
            updatePaneTerminalDisplay(currentPane);
            updatePaneCursor(currentPane);
            
            guiLogger.log("[DEBUG](ClientGUI::processPaneInput) Input processed. Current input: " + currentInput);
        }
    }
}

void ClientGUI::handlePaneSpecialInput(sf::Event event, Pane& currentPane) {
    if (event.type == sf::Event::KeyPressed) {
        std::string currentPath = currentPane.backend->GetPath() + "> ";
        size_t inputLength = currentPane.currentInput.length() - currentPath.length();
        
        switch (event.key.code) {
            case sf::Keyboard::Right:
                // Move cursor right up to end of input
                if (currentPane.cursorPosition < inputLength) {
                    currentPane.cursorPosition++;
                }
                break;
                
            case sf::Keyboard::Left:
                // Move cursor left, but not before shell path
                if (currentPane.cursorPosition > 0) {
                    currentPane.cursorPosition--;
                }
                break;
                
            case sf::Keyboard::Up:
                navigatePaneCommandHistory(currentPane, true);
                break;
                
            case sf::Keyboard::Down:
                navigatePaneCommandHistory(currentPane, false);
                break;
                
            default:
                break;
        }
        
        updatePaneCursor(currentPane);
    }
}

void ClientGUI::navigatePaneCommandHistory(Pane& currentPane, bool goUp) {
    // Get current path with prompt
    std::string currentPath = currentPane.backend-> GetPath() + "> ";
    
    // Check if command history is empty
    if (currentPane.commandHistory.empty()) {
        guiLogger.log("[DEBUG](ClientGUI::navigatecommandHistory) No commands in pane history.");
        return;
    }
    
    // Navigate through command history
    if (goUp) {
        // If no index selected, start from the last command
        if (currentPane.currentHistoryIndex == -1) {
            currentPane.currentHistoryIndex = currentPane.commandHistory.size() - 1;
        }
        // Move to previous command if possible
        else if (currentPane.currentHistoryIndex > 0) {
            currentPane.currentHistoryIndex--;
        }
    } else {
        // If no index selected, do nothing
        if (currentPane.currentHistoryIndex == -1) {
            return;
        }
        
        // Move to next command
        if (currentPane.currentHistoryIndex < currentPane.commandHistory.size() - 1) {
            currentPane.currentHistoryIndex++;
        }
        // Reset to initial state if at the end of history
        else {
            currentPane.currentHistoryIndex = -1;
        }
    }
    
    // Update input based on selected history item
    if (currentPane.currentHistoryIndex != -1) {
        // Construct full command with path
        std::string fullCommand = currentPath +
        currentPane.commandHistory[currentPane.currentHistoryIndex];
        
        // Update pane input
        currentPane.currentInput = fullCommand;
        currentPane.inputText.setString(fullCommand);
        
        // Set cursor position to end of command
        currentPane.cursorPosition =
        currentPane.commandHistory[currentPane.currentHistoryIndex].length();
        
        guiLogger.log("[DEBUG](ClientGUI::navigatecommandHistory) Pane command selected: '" +
                      currentPane.commandHistory[currentPane.currentHistoryIndex] +
                      "'. Index: " + std::to_string(currentPane.currentHistoryIndex));
    } else {
        // Reset to initial state
        currentPane.currentInput = currentPath;
        currentPane.inputText.setString(currentPath);
        currentPane.cursorPosition = 0.0f;
        
        guiLogger.log("[DEBUG](ClientGUI::navigatecommandHistory) Pane command history reset.");
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
            return;
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

void ClientGUI::renderDefaultTerminal() {
    // Ensure the window is clear
    window.clear(sf::Color::Black);
    
    // Update terminal components
    try {
        // Update terminal display
        updateTerminalDisplay();
        
        // Update scroll bar
        updateScrollBar();
        
        // Update cursor position
        updateCursor();
    } catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::renderDefaultTerminal) Update components failed: " +
                      std::string(e.what()));
    }
    
    // Render output text if not empty
    if (!outputText.getString().isEmpty()) {
        try {
            window.draw(outputText);
        } catch (const std::exception& e) {
            guiLogger.log("[ERROR](ClientGUI::renderDefaultTerminal) Drawing output text failed: " +
                          std::string(e.what()));
        }
    }
    
    // Render input text and cursor
    try {
        window.draw(inputText);
        window.draw(cursor);
    } catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::renderDefaultTerminal) Drawing input text or cursor failed: " +
                      std::string(e.what()));
    }
    
    // Render scrollbar if needed
    try {
        if (scrollBar.getSize().y > 0) {
            window.draw(scrollBar);
        }
    } catch (const std::exception& e) {
        guiLogger.log("[ERROR](ClientGUI::renderDefaultTerminal) Drawing scrollbar failed: " +
                      std::string(e.what()));
    }
    
    // Log rendering completion
    guiLogger.log("[DEBUG](ClientGUI::renderDefaultTerminal) Default terminal rendered. Total terminal lines: " + std::to_string(terminalLines.size()));
}

void ClientGUI::processNormalModeInput(sf::Event event) {
    // Log input event for debugging
    guiLogger.log("[DEBUG](ClientGUI::processNormalModeInput) Event type: " +
                  std::to_string(event.type));
    
    // Pane-specific input processing
    if (!panes.empty()) {
        Pane& currentPane = panes[currentPaneIndex];
        
        // Text input for panes
        if (event.type == sf::Event::TextEntered) {
            try {
                processPaneInput(event, currentPane);
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::processNormalModeInput) Pane text input failed: " +
                              std::string(e.what()));
            }
        }
        
        // Special key handling for panes
        if (event.type == sf::Event::KeyPressed) {
            try {
                handlePaneSpecialInput(event, currentPane);
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::processNormalModeInput) Pane special input failed: " +
                              std::string(e.what()));
            }
        }
    }
    // Default terminal input processing
    else {
        // Text input for default terminal
        if (event.type == sf::Event::TextEntered) {
            try {
                processInput(event);
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::processNormalModeInput) Default terminal text input failed: " +
                              std::string(e.what()));
            }
        }
        
        // Special key handling for default terminal
        if (event.type == sf::Event::KeyPressed) {
            try {
                handleSpecialInput(event);
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::processNormalModeInput) Default terminal special input failed: " +
                              std::string(e.what()));
            }
        }
    }
}

void ClientGUI::handleScrolling(sf::Event event) {
    // Static accumulator to manage smooth scrolling
    static float scrollAccumulator = 0.0f;
    
    // Add current scroll delta to accumulator
    scrollAccumulator += event.mouseWheelScroll.delta;
    
    // Log scroll event
    guiLogger.log("[DEBUG](ClientGUI::handleScrolling) Scroll delta: " +
                  std::to_string(event.mouseWheelScroll.delta) +
                  ", Accumulator: " + std::to_string(scrollAccumulator));
    
    // Trigger scroll on significant movement
    if (std::abs(scrollAccumulator) >= 1.0f) {
        int scrollLines = static_cast<int>(scrollAccumulator);
        
        try {
            if (scrollLines > 0) {
                // Scroll up
                scrollUp(1);
                guiLogger.log("[DEBUG](ClientGUI::handleScrolling) Scrolled up");
            } else if (scrollLines < 0) {
                // Scroll down
                scrollDown(1);
                guiLogger.log("[DEBUG](ClientGUI::handleScrolling) Scrolled down");
            }
        } catch (const std::exception& e) {
            guiLogger.log("[ERROR](ClientGUI::handleScrolling) Scroll failed: " + std::string(e.what()));
        }
        
        // Reset accumulator
        scrollAccumulator = 0.0f;
    }
}

void ClientGUI::updateCursorVisibility(bool visible) {
    // Log cursor visibility change
    guiLogger.log("[DEBUG](ClientGUI::updateCursorVisibility) Visibility: " +
                  std::string(visible ? "Visible" : "Hidden"));
    
    // Pane mode cursor handling
    if (!panes.empty()) {
        for (size_t i = 0; i < panes.size(); ++i) {
            Pane& pane = panes[i];
            
            if (i == currentPaneIndex) {
                // Active pane cursor
                pane.cursor.setFillColor(visible ?
                                         sf::Color::White :    // Visible state
                                         sf::Color::Transparent // Hidden state
                                         );
            } else {
                // Inactive pane cursors (dimmed)
                pane.cursor.setFillColor(
                                         sf::Color(100, 100, 100, visible ? 100 : 0)
                                         );
            }
        }
    }
    // Default terminal cursor handling
    else {
        cursor.setFillColor(visible ?
                            sf::Color::White :    // Visible state
                            sf::Color::Transparent // Hidden state
                            );
    }
}


void ClientGUI::handlePaneShortcuts(sf::Event event) {
    // Log the shortcut attempt for debugging
    guiLogger.log("[DEBUG](ClientGUI::handlePaneShortcuts) Shortcut pressed: " +
                  std::to_string(event.key.code));
    
    switch(event.key.code) {
        case sf::Keyboard::H: // Horizontal split
            try {
                createNewPane(SplitType::HORIZONTAL);
                guiLogger.log("[INFO](ClientGUI::handlePaneShortcuts) Created horizontal pane split");
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::handlePaneShortcuts) Failed to create horizontal pane: " +
                              std::string(e.what()));
            }
            break;
            
        case sf::Keyboard::V: // Vertical split
            try {
                createNewPane(SplitType::VERTICAL);
                guiLogger.log("[INFO] Created vertical pane split");
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::handlePaneShortcuts) Failed to create vertical pane: " +
                              std::string(e.what()));
            }
            break;
            
        case sf::Keyboard::Num0: // Switch panes
        {
            if (panes.size() > 1) {
                switchPane(-1); // Move backwards
                guiLogger.log("[DEBUG](ClientGUI::handlePaneShortcuts) Switched to previous pane");
            }
            break;
        }
            
        case sf::Keyboard::Num1: // Switch panes
        {
            if (panes.size() > 1) {
                switchPane(1); // Move forward
                guiLogger.log("[DEBUG](ClientGUI::handlePaneShortcuts) Switched to next pane");
            }
            break;
        }
            
        case sf::Keyboard::W: // Close current pane
            if (!panes.empty()) {
                closeCurrentPane();
                guiLogger.log("[INFO](ClientGUI::handlePaneShortcuts) Closed current pane");
            }
            break;
            
        default:
            break;
    }
    renderPanes();
}

void ClientGUI::run() {
    // Set up frame rate control
    window.setFramerateLimit(60);  // Limit to 60 FPS
    
    // Timing and clock management
    sf::Clock frameClock;
    sf::Clock cursorBlinkClock;
    sf::Clock eventClock;
    
    // Frame and event timing
    const sf::Time frameTime = sf::seconds(1.0f / 60.0f);
    const sf::Time blinkInterval = sf::seconds(0.5f);
    
    // Cursor visibility state
    bool cursorVisible = true;
    
    // Log start of run method
    guiLogger.log("[DEBUG](ClientGUI::run) Starting GUI run loop. " +
                  std::string("Panes: ") + std::to_string(panes.size()) +
                  ", Current mode: " +
                  (currentMode == editorMode::NORMAL ? "Normal" : "Editing"));
    
    if(!panes.empty())
    {
        renderPanes();
    }
    
    // Main application loop
    while (window.isOpen()) {
        
        if (!panes.empty()) {
            renderPanes();
        }
        
        // Event processing
        sf::Event event;
        while (window.pollEvent(event)) {
            // Handle window close event
            if (event.type == sf::Event::Closed) {
                guiLogger.log("[INFO](ClientGUI::run) Window close requested.");
                window.close();
                return;
            }
            
            // Pane management shortcuts (Ctrl + key)
            if (event.type == sf::Event::KeyPressed &&
                sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                try {
                    handlePaneShortcuts(event);
                    if (!panes.empty()) {
                        renderPanes();
                    }
                    
                } catch (const std::exception& e) {
                    guiLogger.log("[ERROR](ClientGUI::run) Pane shortcut error: " +
                                  std::string(e.what()));
                }
            }
            
            // Mode-based input processing
            if (currentMode == editorMode::NORMAL) {
                try {
                    processNormalModeInput(event);
                } catch (const std::exception& e) {
                    guiLogger.log("[ERROR](ClientGUI::run) Normal mode input error: " +
                                  std::string(e.what()));
                }
            }
            else if (currentMode == editorMode::EDITTING) {
                try {
                    processNanoInput(event);
                } catch (const std::exception& e) {
                    guiLogger.log("[ERROR](ClientGUI::run) Nano editor input error: " +
                                  std::string(e.what()));
                }
            }
            
            // mouse scrolling
            if (!panes.empty() && event.type == sf::Event::MouseWheelScrolled) {
                handlePaneScrolling(event, panes[currentPaneIndex]);
                continue;
            }
            
            // Scroll handling
            if (event.type == sf::Event::MouseWheelScrolled) {
                try {
                    handleScrolling(event);
                } catch (const std::exception& e) {
                    guiLogger.log("[ERROR](ClientGUI::run) Scroll handling error: " +
                                  std::string(e.what()));
                }
            }
        }
        
        // Cursor blinking logic
        if (cursorBlinkClock.getElapsedTime() >= blinkInterval) {
            cursorVisible = !cursorVisible;
            
            try {
                updateCursorVisibility(cursorVisible);
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::run) Cursor visibility update error: " +
                              std::string(e.what()));
            }
            
            cursorBlinkClock.restart();
        }
        
        // Rendering
        if (frameClock.getElapsedTime() >= frameTime) {
            // Clear the window
            window.clear(sf::Color::Black);
            
            // Render based on current mode
            try {
                if (currentMode == editorMode::EDITTING) {
                    // Nano editor rendering
                    refreshNanoDisplay();
                } else {
                    // Pane or default terminal rendering
                    if (!panes.empty()) {
                        renderPanes();
                    } else {
                        renderDefaultTerminal();
                    }
                }
            } catch (const std::exception& e) {
                guiLogger.log("[ERROR](ClientGUI::run) Rendering error: " +
                              std::string(e.what()));
                
                // Fallback rendering
                window.clear(sf::Color::Black);
                sf::Text errorText;
                errorText.setFont(font);
                errorText.setString("Rendering Error: " + std::string(e.what()));
                errorText.setCharacterSize(20);
                errorText.setFillColor(sf::Color::Red);
                window.draw(errorText);
            }
            
            // Display the rendered frame
            window.display();
            
            // Restart frame clock
            frameClock.restart();
        }
    }
    
    // Log end of run method
    guiLogger.log("[INFO](ClientGUI::run) GUI run loop terminated.");
}

}
