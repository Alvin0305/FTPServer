#ifndef FTPSERVER_CONSTANTS_HPP
#define FTPSERVER_CONSTANTS_HPP

#include <string>
#include <vector>
#include <cstdint>

#define PORT 8080
#define SERVER_QUEUE 10
#define BUFFER_SIZE 4096

#define DB_FILE "../keys/users.db"
#define TEMP_DB_FILE "../keys/temp.db"
#define ENV_FILE "../keys/.env"

#define TEMP_FILE "temp.tmp"

struct Request {
    std::string command;
    std::vector<std::string> args;
};

enum ClientState {
    CONTINUE,
    STOP
};

enum StatusCode {
    OK = 200,
    BAD_REQUEST = 400,
    NOT_AUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    SERVER_ERROR = 500,
};

struct ReturnHeader {
    StatusCode status;
    std::string header;
    uintmax_t size;
    std::string arg;
};

#endif //FTPSERVER_CONSTANTS_HPP