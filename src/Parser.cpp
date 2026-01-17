#include "Parser.hpp"

#include <cinttypes>
#include <iostream>
#include <sstream>

std::string &Parser::l_strip(std::string &input) {
    input.erase(input.begin(), std::find_if(input.begin(), input.end(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }));

    return input;
}

std::string &Parser::r_strip(std::string &input) {
    input.erase(std::find_if(input.rbegin(), input.rend(), [](const unsigned char ch) {
        return !std::isspace(ch);
    }).base(), input.end());

    return input;
}

std::string &Parser::strip(std::string &input) {
    return r_strip(l_strip(input));
}

Request Parser::parseRequest(const std::string &input) {
    std::vector<std::string> args;
    std::stringstream ss(input);
    std::string head;
    std::string token;

    getline(ss, head, ' ');
    std::transform(head.begin(), head.end(), head.begin(), toupper);

    while (getline(ss, token, ' ')) {
        args.push_back(strip(token));
    }

    return {head, args};
}

ReturnHeader Parser::parseReturnHeader(const std::string &input) {
    std::string temp;
    std::string responseType;
    std::string arg;

    std::stringstream ss(input);

    ss >> temp;
    const auto code = static_cast<StatusCode>(std::atoi(temp.c_str()));

    ss >> responseType;

    ss >> temp;
    char *end = nullptr;
    const uintmax_t size = std::strtoumax(temp.c_str(), &end, 10);

    if (ss >> std::ws && !ss.eof()) {
        std::getline(ss, arg);
    }

    return {code, responseType, size, arg};
}
