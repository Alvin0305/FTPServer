#ifndef FTPSERVER_AUTHHANDLER_H
#define FTPSERVER_AUTHHANDLER_H
#include <string>

class AuthHandler {
    explicit AuthHandler();
public:
    static bool userExists(const std::string &username);
    static void updatePassword(const std::string &username, const std::string &password);
    static void saveUser(const std::string &username, const std::string &password);
    static std::string hashPassword(const std::string &password);
    static bool verifyPassword(const std::string &password, const std::string &storedHash);
    static bool isAuthenticated(const std::string &username, const std::string &password);

private:
    static int lockFile(const char *path, int flags, int lockType);
    static void unlockFile(int fd);
};

#endif //FTPSERVER_AUTHHANDLER_H
