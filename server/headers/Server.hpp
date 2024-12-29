//
//  Server.hpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#pragma once

#include "../headers/Logger.hpp"

// std
#include <string>
#include <mutex>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <netinet/in.h>
#include <cstdio>
#include <filesystem>
#include <cstring>
#include <stdexcept>

using namespace std::filesystem;

namespace server {

class Server {
public:
    Server(unsigned short port);
    ~Server();
    
    // deactivate copy operator overload
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    
    void run();
    
private:
    int serverSocket;
    unsigned short port;
    std::mutex clientMutex; // Mutex for syncing the access to the resources
    logs::Logger logger;
    
    void handleClient(int clientSocket);
    
    // cd functions to handle edge cases and ensure proper functionality
    bool validateDirectory(const path& targetPath);
    path resolvePath(const std::string& rawPath);
    void handleChangeDirectory(const std::string& command, int clientSocket);
    
    //  clean client command
    std::string cleanedCommand(std::string& command);
    
    // functions to handle other commands execution
    std::string executeCommand(const std::string& cmd);
    void processCommand(const std::string& command, int clientSocket, std::string& outputBuffer);
    
    // nano editor functions
    std::string handleNanoCommand(const std::string& command);
    
};

}
