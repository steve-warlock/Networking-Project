//
//  Client.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "../../headers/Client.hpp"

namespace client {

Client::Client(std::string serverIP, unsigned short port) : backend_logic(serverIP, port), gui_logic(backend_logic){}

void Client::run() {
    gui_logic.run();
}

}

