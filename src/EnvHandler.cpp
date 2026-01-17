#include "EnvHandler.hpp"

#include <fstream>
#include <iostream>

#include "constants.hpp"

std::unordered_map<std::string, std::string> EnvHandler::envVariables{};

void EnvHandler::load() {
    std::ifstream inFile(ENV_FILE);
    if (!inFile.is_open()) {
        std::cout << "Creating .env file" << std::endl;
        std::ofstream outFile(ENV_FILE);
        if (!outFile.is_open()) {
            std::cout << "Creating .env file failed" << std::endl;
        }
        outFile << "DOWNLOAD_DIR=../downloads" << std::endl;
        inFile.open(ENV_FILE);
    }
    std::string line;

    while (getline(inFile, line)) {
        const auto pos = line.find('=');
        if (pos != std::string::npos) {
            envVariables[line.substr(0, pos)] = line.substr(pos + 1);
        }
    }

    inFile.close();
}

void EnvHandler::save() {
    std::ofstream outFile(ENV_FILE);

    for (const auto &[key, value]: envVariables) {
        outFile << key << "=" << value << std::endl;
    }

    outFile.close();
}

std::optional<std::string> EnvHandler::getEnv(const std::string &key) {
    const auto it = envVariables.find(key);
    if (it != envVariables.end()) {
        return it->second;
    }
    return std::nullopt;
}

void EnvHandler::setEnv(const std::string &key, const std::string &value) {
    envVariables[key] = value;
}
