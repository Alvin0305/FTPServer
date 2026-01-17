#include "Server.hpp"

#include <csignal>

#include "constants.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>

#include "AuthHandler.hpp"
#include "ClientHandler.hpp"

Server::~Server() {
    close(socket_fd);
}

Server::Server() {
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    constexpr int opt = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(socket_fd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, SERVER_QUEUE) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server started at port " << PORT << "..." << std::endl;
}

void Server::run() const {
    while (true) {
        int clientSocket = accept(socket_fd, nullptr, nullptr);
        if (clientSocket < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "Client connected " << clientSocket << std::endl;

        std::thread clientHandlerThread(handleClient, clientSocket);
        clientHandlerThread.detach();
    }
}

void Server::handleClient(const int clientSocket) {
    ClientHandler clientHandler(clientSocket);
    clientHandler.run();
}

int main() {
    const Server server;
    signal(SIGPIPE, SIG_IGN);
    AuthHandler::saveUser("alvin", "abcd");
    server.run();
    return 0;
}