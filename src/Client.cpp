#include "Client.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "constants.hpp"
#include "EnvHandler.hpp"
#include "Parser.hpp"
#include "Util.hpp"

Client::Client(const std::string &serverIp, const int port) {
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(socket_fd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to the server " << serverIp << ":" << port << std::endl;
}

void Client::sendRequest(std::string request) const {
    request.append("\n");
    if (send(socket_fd, request.c_str(), request.length(), 0) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

ReturnHeader Client::extractHeader() {
    ssize_t n;
    char buffer[BUFFER_SIZE];

    size_t pos;
    if ((pos = recvBuffer.find('\n')) != std::string::npos) {
        const std::string headerString = recvBuffer.substr(0, pos);

        recvBuffer.erase(0, pos + 1);
        return Parser::parseReturnHeader(headerString);
    }

    while ((n = recv(socket_fd, buffer, BUFFER_SIZE, 0)) > 0) {
        recvBuffer.append(buffer, n);

        if ((pos = recvBuffer.find('\n')) != std::string::npos) {
            const std::string headerString = recvBuffer.substr(0, pos);

            recvBuffer.erase(0, pos + 1);
            return Parser::parseReturnHeader(headerString);
        }
    }
    return {};
}

std::string Client::receiveResponse(const ReturnHeader &header) {
    const uintmax_t totalExpected = header.size;
    uintmax_t totalReceived = 0;
    char buffer[BUFFER_SIZE];
    std::string result;

    if (!recvBuffer.empty()) {
        const size_t take = std::min(static_cast<size_t>(totalExpected), recvBuffer.length());
        result.append(recvBuffer, 0, take);
        totalReceived += take;
        recvBuffer.erase(0, take);
    }

    while (totalReceived < totalExpected) {
        const ssize_t received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (received <= 0) break;

        const auto needed = totalExpected - totalReceived;
        if (static_cast<size_t>(received) <= needed) {
            result.append(buffer, received);
            totalReceived += received;
        } else {
            result.append(buffer, needed);
            recvBuffer.append(buffer + needed, received - needed);
            totalReceived += needed;
            // break;
        }
    }

    return result;
}

void Client::receiveFile(const ReturnHeader &header) {
    const uintmax_t totalExpected = header.size;
    uintmax_t totalReceived = 0;
    char buffer[BUFFER_SIZE];

    auto optDownloadDir = EnvHandler::getEnv("DOWNLOAD_DIR");
    if (!optDownloadDir) {
        std::cout << "Download directory not set." << std::endl;
        exit(EXIT_FAILURE);
    }

    const std::string &downloadDir = *optDownloadDir;

    std::filesystem::path downloadFile = std::filesystem::path(downloadDir) / header.arg;
    std::filesystem::create_directories(downloadDir);

    std::ofstream outFile(downloadFile, std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "Error: Could not open " << downloadFile << " for writing" << std::endl;
        return;
    }

    if (!recvBuffer.empty()) {
        size_t take = std::min(static_cast<size_t>(totalExpected), recvBuffer.length());
        totalReceived += take;
        outFile.write(recvBuffer.c_str(), static_cast<std::streamsize>(take));
        recvBuffer.erase(0, take);
    }

    while (totalReceived < totalExpected) {
        ssize_t received = recv(socket_fd, buffer, BUFFER_SIZE, 0);

        if (received <= 0) {
            std::cout << "Connection lost during file transfer." << std::endl;
            break;
        }

        size_t needed = totalExpected - totalReceived;

        if (received <= needed) {
            outFile.write(buffer, received);
            totalReceived += received;
        } else {
            outFile.write(buffer, static_cast<std::streamsize>(needed));
            recvBuffer.append(buffer + needed, received - needed);
            totalReceived += received;
        }
        Util::showProgressBar(totalReceived, totalExpected);
    }

    outFile.close();
    std::cout << "\nFile Received: " << downloadFile << std::endl;
    consumeEnd();
}

void Client::catFile(const ReturnHeader &header) {
    const uintmax_t totalExpected = header.size;
    uintmax_t totalReceived = 0;
    char buffer[BUFFER_SIZE];

    if (!recvBuffer.empty()) {
        const size_t take = std::min(static_cast<size_t>(totalExpected), recvBuffer.length());
        totalReceived += take;
        std::cout << recvBuffer.substr(0, take);
        recvBuffer.erase(0, take);
    }

    while (totalReceived < totalExpected) {
        const ssize_t received = recv(socket_fd, buffer, BUFFER_SIZE, 0);

        if (received <= 0) {
            std::cout << "Connection lost during file transfer." << std::endl;
            break;
        }

        const size_t needed = totalExpected - totalReceived;

        if (received <= needed) {
            std::cout << std::string(buffer, received);
            totalReceived += received;
        } else {
            std::cout << std::string(buffer, needed);
            recvBuffer.append(buffer + needed, received - needed);
            totalReceived += received;
        }
    }

    consumeEnd();
}

void Client::consumeEnd() {
    constexpr int size = 5;
    unsigned long received = recvBuffer.length();
    char buffer[BUFFER_SIZE];

    while (received < size) {
        const ssize_t n = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        recvBuffer.append(buffer, n);

        received += n;
    }

    std::string response = recvBuffer.substr(0, size);
    recvBuffer.erase(0, size);
}

bool Client::sendFile(const Request &request) const {
    std::filesystem::path path = request.args[0];
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Error: Could not open " << request.args[0] << std::endl;
        return false;
    }

    const uintmax_t fileSize = std::filesystem::file_size(path);
    uintmax_t totalSend = 0;
    std::string updateReq = "PUT_ " + request.args[0] + " " + std::to_string(fileSize);
    sendRequest(updateReq);

    char buffer[BUFFER_SIZE];
    while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0) {
        std::streamsize bytesRead = file.gcount();
        totalSend += bytesRead;

        Util::showProgressBar(totalSend, fileSize);

        if (bytesRead > 0) {
            if (send(socket_fd, buffer, bytesRead, 0) <= 0) {
                std::cout << "Error writing to socket" << std::endl;
                file.close();
                return false;
            }
        }
    }

    std::cout << std::endl;
    file.close();
    return true;
}

void Client::run() {
    while (true) {
        std::string line;
        std::cout << "> ";
        getline(std::cin, line);

        const Request request = Parser::parseRequest(line);

        if (request.command == "SDD") {
            if (request.args.size() != 1) {
                std::cout << "Error: sdd <path>" << std::endl;
            } else {
                EnvHandler::setEnv("DOWNLOAD_DIR", request.args[0]);
                std::cout << "download directory changed" << std::endl;
            }
            continue;
        }

        if (request.command == "PDD") {
            std::optional<std::string> optDir = EnvHandler::getEnv("DOWNLOAD_DIR");
            if (!optDir) {
                std::cout << "download directory is not set" << std::endl;
            } else {
                std::cout << "download directory is: " << *optDir << std::endl;
            }
            continue;
        }

        if (request.command == "PUT") {
            sendRequest(line);
            const ReturnHeader header = extractHeader();
            const std::string response = receiveResponse(header);
            consumeEnd();

            if (header.status != OK) {
                std::cout << response << std::endl;
            } else if (sendFile(request)) {
                ReturnHeader putReturnHeader = extractHeader();
                std::cout << receiveResponse(putReturnHeader) << std::endl;
                consumeEnd();
            }
            continue;
        }

        sendRequest(line);
        ReturnHeader header = extractHeader();

        if (header.header == "QUIT") {
            break;
        }

        if (header.header == "GET" && header.status == OK) {
            receiveFile(header);
            continue;
        }

        if (header.header == "CAT" && header.status == OK) {
            catFile(header);
            continue;
        }

        std::cout << receiveResponse(header) << std::endl;
        consumeEnd();
    }

    std::cout << "Closing connection with server" << std::endl;
    close(socket_fd);
}

int main(const int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ftpclient <server_ip> <server_port>" << std::endl;
        return 1;
    }

    const std::string serverIp = argv[1];
    const int port = std::stoi(argv[2]);

    EnvHandler::load();
    Client client(serverIp, port);
    client.run();
    EnvHandler::save();
}
