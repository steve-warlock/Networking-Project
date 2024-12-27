//
//  Logger.hpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#pragma once

// std
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <iostream>
#include <filesystem>

namespace logs {

class Logger {
private:
    std::string logFilePath;
    std::ofstream logFile;
    std::mutex logMutex;          // Mutex for thread-safe access

public:
    Logger(const std::string& filePath);
    ~Logger();
    void log(const std::string& message);
};

}

