//
//  ClientBackend.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "../../headers/ClientBackend.hpp"

namespace backend {

ClientBackend::ClientBackend(const std::string& ip, unsigned short port) : logger("./client_backend.log") {
    
    this -> clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    if(-1 == this -> clientSocket) {
        logger.log("[ERROR](ClientBackend::ClientBackend) Failed to create client socket.");
        throw "Failed to create client socket.";
    }
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        close(clientSocket);
        this -> logger.log("[ERROR](ClientBackend::ClientBackend) Invalid server address.");
        throw "Invalid server address.";
    }
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1){
        close(clientSocket);
        this -> logger.log("[ERROR](ClientBackend::ClientBackend) Failed to connect to server.");
        throw "Failed to connect to server.";
    }
    
    this -> logger.log("[DEBUG](ClientBackend::ClientBackend) Client with id " + std::to_string(clientSocket) + " connected to server at " + ip + ":" + std::to_string(port));
}

ClientBackend::~ClientBackend()
{
    this -> logger.log("[DEBUG](ClientBackend::~ClientBackend) Socket with id " + std::to_string(this -> clientSocket) + " closed and ClientBackend destroyed with.");
    close(this -> clientSocket);
    
}

std::string ClientBackend::sendCommand(const std::string &command) {
    std::lock_guard<std::mutex> lock(this -> backendMutex); // Protect access to the socket
    
    logger.log("[DEBUG](ClientBackend::sendCommand) Sending command to server: " + command);
    
    if(command.length() > 1024){
        this -> logger.log("[ERROR](ClientBackend::sendCommand) Command too long.");
        throw std::length_error("Command too long.");
    }
    
    if(-1 == send(clientSocket, command.c_str(), command.size(), 0)){
        this -> logger.log("[ERROR](ClientBackend::sendCommand) Failed to send command to server.");
        throw "Failed to send command to server.";
    }
    
    std::vector<char> buffer(8192, '\0');
    
    ssize_t bytesRead = recv(clientSocket, buffer.data(), buffer.size() - 1, 0);
    
    if (bytesRead <= 0) {
        logger.log("[ERROR](ClientBackend::sendCommand) Failed to receive response from server.");
        throw "Failed to receive response from server.";
    }
    
    buffer[bytesRead] = '\0';
    std::string response(buffer.data());
    
    logger.log("[DEBUG](ClientBackend::sendCommand) Received response from server: " + response);
    
    return response;
}

void ClientBackend::SetPath(std::string& new_path){
    std::lock_guard<std::mutex> lock(this -> pathMutex);
    this -> currentPath = new_path;
}

std::string ClientBackend::GetPath() const {
    std::lock_guard<std::mutex> lock(this -> pathMutex);
    return this -> currentPath;
}

}
