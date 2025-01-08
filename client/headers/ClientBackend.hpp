//
//  ClientBackend.hpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#pragma once

// logger
#include "Logger.hpp"

// std
#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <memory>
#include <stdexcept>
#include <filesystem>

namespace backend {

class ClientBackend{
    
public:
    
    ClientBackend(const std::string& ip, unsigned short port);
    ~ClientBackend();
    
    std::string GetPath() const;
    void SetPath(std::string& new_path);
    
    std::string sendCommand(const std::string& command);
private:
    int clientSocket;
    std::mutex backendMutex; // for protecting the socket access
    mutable std::mutex pathMutex;
    logs::Logger logger;
    std::string currentPath;
};

}
