//
//  Server.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "../headers/Server.hpp"

namespace server {

// cd functions
bool Server::validateDirectory(const path &targetPath){
    try {
        // check if the path exists
        if(!exists(targetPath)){
            logger.log("[ERROR](Server::validateDirectory) The path does not exists: " + targetPath.string());
            return false;
        }
        
        // check if it is a directory
        if(!is_directory(targetPath)){
            logger.log("[ERROR](Server::validateDirectory) The path is not a directory: " + targetPath.string());
            return false;
        }
        
        // check for privileges (read, execute)
        if(-1 == access(targetPath.string().c_str(), R_OK | X_OK)){
            logger.log("[ERROR](Server::validateDirectory) The path does not have read/execute permissions: " + targetPath.string());
            return false;
        }
        
        return true;
    } catch (const filesystem_error& e) {
        logger.log("[ERROR](Server::validateDirectory) An error occurred: " + std::string(e.what()));
        return false;
    }
}

path Server::resolvePath(const std::string &rawPath){
    path resolvedPath;
    
    // home directory (cd or cd ~)
    if(rawPath.empty() || rawPath == "~") {
        // get the HOME env
        const char* home = std::getenv("HOME");
        resolvedPath = home ? home : "/"; // go root if not found HOME
    }
    
    // expand ~/ -> $HOME/
    else if(rawPath[0] == '~' && rawPath.length() > 1 && rawPath[1] == '/') {
        const char* home = std::getenv("HOME");
        resolvedPath = home ? path(home)  / rawPath.substr(2): path(rawPath.substr(2));
    }
    // relative path
    else if(rawPath == ".") {
        resolvedPath = current_path();
    }
    else if(rawPath == "..") {
        resolvedPath = current_path().parent_path();
    }
    // other cases
    else {
        resolvedPath = rawPath[0] == '/' ?
                path(rawPath) : // absolute
                current_path() / rawPath; // relative
    }
    
    resolvedPath = canonical(resolvedPath);
    
    return resolvedPath;
}

void Server::handleChangeDirectory(const std::string &command, int clientSocket){
    std::string rawPath = command.substr(2); // after "cd"
    
    try {
        // trim the path of whitespaces
        rawPath.erase(0, rawPath.find_first_not_of(" \t"));
        rawPath.erase(rawPath.find_last_not_of(" \t") + 1);
        
        path targetPath = resolvePath(rawPath.empty() ? "~" : rawPath);
        
        // validation phase
        if(!validateDirectory(targetPath)) {
            std::string errorMSG = "Invalid directory: " + rawPath;
            logger.log("[ERROR](Server::handleChangeDirectory) " + errorMSG);
            send(clientSocket, errorMSG.c_str(), errorMSG.length(), 0);
            return ;
        }
        
        // change the current directory
        current_path(targetPath);
        
        std::string msg = "\n" + targetPath.string();
        send(clientSocket, msg.c_str(), msg.length(), 0);
        
        logger.log("[DEBUG](Server::handleChangeDirectory) Changed directory to: " + targetPath.string());
        
    } catch(const filesystem_error& e) {
        std::string errorMsg = "Filesystem error: " + std::string(e.what());
        logger.log("[ERROR](Server::handleChangeDirectory) " + errorMsg);
        send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
    } catch(const std::exception& e) {
        std::string errorMsg = "Exception error: " + std::string(e.what());
        logger.log("[ERROR](Server::handleChangeDirectory) " + errorMsg);
        send(clientSocket, errorMsg.c_str(), errorMsg.length(), 0);
    }
}

std::string Server::executeCommand(const std::string &cmd) {
    // security concerns
    if(cmd.find("sudo") != std::string::npos) {
        logger.log("[SECURITY](Server::executeCommand) Blocked sudo command: " + cmd);
        return "Error: sudo commands are not allowed";
    }
    
    std::vector<char> buffer(4096, '\0');
    std::string result;

    // Execute command
    FILE* pipe = popen(("/bin/bash -c '" + cmd + " 2>&1'").c_str(), "r");
    if (!pipe) {
        logger.log("[ERROR](Server::executeCommand) Failed to execute command: " + cmd);
        return "Error: Command not recognized or failed to execute.";
    }

    bool hasOutput = false;
    while (fgets(buffer.data(), (int)buffer.size(), pipe) != nullptr) {
        result.append(buffer.data());
        hasOutput = true;
    }

    int status = pclose(pipe);
    if (status != 0) {
        logger.log("[WARN](Server::executeCommand) Command returned non-zero status: " + cmd);
        
        // no output means error message
        if (!hasOutput) {
            return "Error: Unknown command: " + cmd;
        }
    }

    // edge case
    if (result.empty()) {
        return "Warn: Command executed but produced no output.";
    }

    return result;
}

// nano
std::string Server::handleNanoCommand(const std::string& command) {
    logger.log("[DEBUG](Server::handleNanoCommand) Received nano command: " + command);

    std::string filename = command.substr(5);
    
    try {
       
        std::filesystem::path filePath = current_path() / filename;

        logger.log("[DEBUG](Server::handleNanoCommand) Resolved file path: " + filePath.string());

        if (!std::filesystem::exists(filePath)) {
            std::ofstream newFile(filePath);
            newFile.close();
            logger.log("[DEBUG](Server::handleNanoCommand) Created new file: " + filePath.string());
            return "";
        }

        std::ifstream file(filePath);
        std::string content(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        logger.log("[INFO](Server::handleNanoCommand) Successfully read file: " + filePath.string() +
                   ", Content length: " + std::to_string(content.length()) + " bytes");

        return content;

    } catch (const std::exception& e) {
        logger.log("[ERROR](Server::handleNanoCommand) Failed to handle nano command: " + std::string(e.what()));
        return "Error: Cannot open file";
    }
}

void Server::processCommand(const std::string &command, int clientSocket, std::string &outputBuffer){
    try {
        // special command
        if(command.substr(0,2) == "cd") {
            handleChangeDirectory(command, clientSocket);
            return;
        }
        
        if(command.substr(0,4) == "nano") {
            outputBuffer = handleNanoCommand(command);
            send(clientSocket, outputBuffer.c_str(), outputBuffer.length(), 0);
            return;
        }
        
        // execute standard commands
        outputBuffer = executeCommand(command);
    } catch (const std::exception& e) {
        logger.log("[ERROR](Server::processCommand) Command processing error: " + std::string(e.what()));
        outputBuffer = "Error: " + std::string(e.what());
    }
}

Server::Server(unsigned short port) : port(port), logger("./server.log") {
    logger.log("[DEBUG](Server::Server) Initializing server...");
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        logger.log("[ERROR](Server::Server) Failed to create socket.");
        exit(1);
    }
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(this -> port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        logger.log("[ERROR](Server::Server) Failed to bind socket.");
        close(serverSocket);
        exit(1);
    }
    
    if (listen(serverSocket, 10) == -1) {
        logger.log("[ERROR](Server::Server) Failed to listen on socket.");
        close(serverSocket);
        exit(1);
    }
    
    logger.log("[DEBUG](Server::Server) Server listening on port " + std::to_string(port) + ".");
}

Server::~Server() {
    close(serverSocket);
    logger.log("[DEBUG](Server::~Server) Server socket closed.");
}

void Server::run() {
    logger.log("[DEBUG](Server::run) Starting server loop.");
   
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == -1) {
            logger.log("[ERROR](Server::run) Failed to accept client connection.");
            continue;
        }
        
        logger.log("[DEBUG](Server::run) Client connected with id: " + std::to_string(clientSocket));
        std::thread(&Server::handleClient, this, clientSocket).detach();
    }
}

std::string Server::cleanedCommand(std::string& command){
    std::string cleanCommand;
    for(auto c : command)
    {
        if(c != '\n' && c != '\r' && c != '\b'){
            cleanCommand.push_back(c);
        }
    }
    return cleanCommand;
}

void Server::handleClient(int clientSocket) {
    char buffer[1024] = {'\0'};
    ssize_t bytesRead;

    logger.log("[DEBUG](Server::handleClient) Handling new client with id " + std::to_string(clientSocket) + ".");
    
    while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        std::string command(buffer);

        logger.log("[DEBUG](Server::handleClient) Received command: " + command);

        // Trim trailing newline and backspaces characters from the command
        command = cleanedCommand(command);

        if (command == "exit") {
            logger.log("[DEBUG](Server::handleClient) Exit command received. Closing client with id " + std::to_string(clientSocket) + " connection.");
            std::string response = "Goodbye!";
            send(clientSocket, response.c_str(), response.size(), 0);
            break;
        }
        
        std::string outputBuffer;
        
        // Specific handling for nano command
        processCommand(command, clientSocket, outputBuffer);

        // Send the output or error response back to the client
        {
            std::lock_guard<std::mutex> lock(clientMutex);
            logger.log("[DEBUG](Server::handleClient) Sending response: " + outputBuffer);
            
            send(clientSocket, outputBuffer.c_str(), outputBuffer.size(), 0);
        }
    
        memset(buffer, '\0', sizeof(buffer));
    }
   
    // mutex deconnecting
    {
        std::lock_guard<std::mutex>lock(clientMutex);
        if (bytesRead == 0) {
            logger.log("[DEBUG](Server::handleClient) Client disconnected.");
        } else if (bytesRead < 0) {
            logger.log("[ERROR](Server::handleClient) Failed to receive data from client.");
        }
    }
        
    close(clientSocket);
    logger.log("[DEBUG](Server::handleClient) Client socket closed.");
}

}
