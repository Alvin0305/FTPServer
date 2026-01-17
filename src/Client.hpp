#ifndef FTPSERVER_CLIENT_H
#define FTPSERVER_CLIENT_H
#include <filesystem>
#include <string>

#include "constants.hpp"

class Client {
public:
    Client(const std::string &serverIp, int port);

    void run();

private:
    int socket_fd;
    std::string recvBuffer;

    bool sendFile(const Request& request) const;
    void sendRequest(std::string request) const;
    std::string receiveResponse(const ReturnHeader &header);
    void receiveFile(const ReturnHeader &header);
    void catFile(const ReturnHeader &header);
    ReturnHeader extractHeader();
    void consumeEnd();
};

#endif //FTPSERVER_CLIENT_H
