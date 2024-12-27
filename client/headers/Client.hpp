//
//  Client.hpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#pragma once

#include "./ClientBackend.hpp"
#include "./ClientGUI.hpp"


namespace client {

class Client {
public:
    Client(std::string serverIP, unsigned short port);
    void run();

private:
    backend::ClientBackend backend_logic;
    gui::ClientGUI gui_logic;
};

}
