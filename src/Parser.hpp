#ifndef FTPSERVER_COMMANDPARSER_H
#define FTPSERVER_COMMANDPARSER_H

#include "Client.hpp"
#include "constants.hpp"

class Parser {
public:
    static Request parseRequest(const std::string &input);
    static ReturnHeader parseReturnHeader(const std::string &input);

private:
    static std::string &l_strip(std::string &input);
    static std::string &r_strip(std::string &input);
    static std::string &strip(std::string &input);
};

#endif //FTPSERVER_COMMANDPARSER_H
