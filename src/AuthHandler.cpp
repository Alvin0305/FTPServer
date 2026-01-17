#include "AuthHandler.hpp"

#include <fstream>
#include <sodium.h>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>

#include "constants.hpp"

AuthHandler::AuthHandler() {
    if (sodium_init() < 0) {
        throw std::runtime_error("Cannot init Sodium");
    }
}

int AuthHandler::lockFile(const char *path, const int flags, const int lockType) {
    const int fd = open(path, flags, 0600);
    if (fd < 0) return -1;
    flock(fd, lockType);
    return fd;
}

void AuthHandler::unlockFile(int fd) {
    flock(fd, LOCK_UN);
    close(fd);
}

bool AuthHandler::isAuthenticated(const std::string &username, const std::string &password) {
    const int fd = lockFile(DB_FILE, O_RDONLY, LOCK_SH);
    std::ifstream file(DB_FILE);

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;

        std::string user = line.substr(0, pos);
        std::string hash = line.substr(pos + 1);

        if (user == username) {
            unlockFile(fd);
            return verifyPassword(password, hash);
        }
    }

    unlockFile(fd);
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
    const int fd = lockFile(DB_FILE, O_RDONLY, LOCK_SH);
    std::ifstream file(DB_FILE);
    std::string line;

    while (std::getline(file, line)) {
        if (line.starts_with(username + ":")) {
            unlockFile(fd);
            return true;
        }
    }

    unlockFile(fd);
    return false;
}

void AuthHandler::saveUser(const std::string &username, const std::string &password) {
    const int fd = lockFile(DB_FILE, O_WRONLY | O_RDONLY | O_CREAT, LOCK_EX);

    std::ofstream file(DB_FILE, std::ios::app);
    file << username << ":" << hashPassword(password) << std::endl;

    unlockFile(fd);
}

void AuthHandler::updatePassword(const std::string &username, const std::string &password) {
    int fd = lockFile(DB_FILE, O_RDWR, LOCK_EX);

    std::ifstream db(DB_FILE);
    std::ofstream temp(TEMP_DB_FILE, std::ios::app);
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

    unlockFile(fd);
}
