#ifndef FTPSERVER_SERVER_H
#define FTPSERVER_SERVER_H

class Server {
public:
    explicit Server();
    ~Server();
    void run() const;

private:
    int socket_fd;

    static void handleClient(int clientSocket);
};

#endif //FTPSERVER_SERVER_H