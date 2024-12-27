//
//  runner_server.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "./headers/Server.hpp"


int main()
{
    try {
        server::Server server(8080);
        server.run();
    } catch (std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}
