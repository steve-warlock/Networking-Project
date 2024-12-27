//
//  runner_client.cpp
//  RemMux
//
//  Created by Steve Warlock on 07.12.2024.
//

#include "./headers/Client.hpp"

int main() {
    try {
        client::Client client("127.0.0.1", 8080);
        client.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

