#ifndef FTPSERVER_CLIENTHANDLER_H
#define FTPSERVER_CLIENTHANDLER_H
#include <filesystem>
#include <string>

#include "constants.hpp"

class ClientHandler {
public:
    explicit ClientHandler(int clientSocket);

    void run();

private:
    int clientSocket;
    bool authenticated;
    bool paired;
    std::filesystem::path currentPath;
    std::string recvBuffer;
    std::string username;

    Request readRequest();
    void sendResponse(StatusCode statusCode, const std::string &responseType, const std::string &data) const;
    void sendFile(const std::filesystem::path& path, const std::string &requestType) const;
    bool saveFileToTemp(const Request &request);

    ClientState commandHandler(const Request &request);
};

#endif //FTPSERVER_CLIENTHANDLER_H
