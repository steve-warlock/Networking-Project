//
//  Logger.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "../../headers/Logger.hpp"

using namespace std::filesystem;

namespace logs {

Logger::Logger(const std::string& fileName) {
    
    this -> logFilePath = path(current_path() / "logs" / fileName).string();
    this -> logFile.open(logFilePath, std::ios::app);
    if (!this -> logFile.is_open()) {
        std::cerr << "Error: Unable to open log file: " << this -> logFilePath << "\n";
                   exit(1);
    }
}

Logger::~Logger() {
    if (this -> logFile.is_open()) {
        this -> logFile.close();
    }
}

void Logger::log(const std::string& message) {
    // Lock for thread-safe access
    std::lock_guard<std::mutex> guard(logMutex);
    
    // Get current timestamp
    std::time_t now = std::time(nullptr);
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    
    // Write timestamp and message in the file
    this -> logFile << "[" << timeBuffer << "]" << message << '\n';
    this -> logFile.flush();
}

}
