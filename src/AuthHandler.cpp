#include "AuthHandler.hpp"

#include <fstream>
#include <sodium.h>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>

#include "constants.hpp"

bool initSodium() {
    static bool initialized = false;
    if (!initialized) {
        if (sodium_init() < 0) {
            return false;
        }
        initialized = true;
    }
    return true;
}

AuthHandler::AuthHandler() {
    initSodium();
}

bool AuthHandler::isAuthenticated(const std::string &username, const std::string &password) {
    std::ifstream file(DB_FILE);

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;

        std::string user = line.substr(0, pos);
        std::string hash = line.substr(pos + 1);

        if (user == username) {
            return verifyPassword(password, hash);
        }
    }

    return false;
}

std::string AuthHandler::hashPassword(const std::string &password) {
    char hash[crypto_pwhash_STRBYTES];

    if (crypto_pwhash_str(hash, password.c_str(), password.size(), crypto_pwhash_OPSLIMIT_MODERATE,
                          crypto_pwhash_MEMLIMIT_MODERATE) != 0) {
        throw std::runtime_error("Cannot hash the password: Out of memory");
    }

    return std::string(hash);
}

bool AuthHandler::verifyPassword(const std::string &password, const std::string &storedHash) {
    return crypto_pwhash_str_verify(storedHash.c_str(), password.c_str(), password.size()) == 0;
}

bool AuthHandler::userExists(const std::string &username) {
    std::ifstream file(DB_FILE);
    std::string line;

    while (std::getline(file, line)) {
        if (line.starts_with(username + ":")) {
            return true;
        }
    }

    return false;
}

void AuthHandler::saveUser(const std::string &username, const std::string &password) {
    std::ofstream file(DB_FILE, std::ios::app);
    file << username << ":" << hashPassword(password) << std::endl;
}

void AuthHandler::updatePassword(const std::string &username, const std::string &password) {
    std::ifstream db(DB_FILE);
    std::ofstream temp(TEMP_DB_FILE);
    std::string line;

    while (std::getline(db, line)) {
        if (line.starts_with(username + ":")) {
            temp << username << ":" << hashPassword(password) << std::endl;
        } else {
            temp << line << std::endl;
        }
    }

    db.close();
    temp.close();

    std::remove(DB_FILE);
    std::rename(TEMP_DB_FILE, DB_FILE);
}
