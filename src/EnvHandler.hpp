#ifndef FTPSERVER_ENVHANDLER_H
#define FTPSERVER_ENVHANDLER_H
#include <optional>
#include <string>
#include <unordered_map>

class EnvHandler {
public:
    static void load();
    static void save();
    static std::optional<std::string> getEnv(const std::string &key);
    static void setEnv(const std::string &key, const std::string &value);

private:
    static std::unordered_map<std::string, std::string> envVariables;
};

#endif //FTPSERVER_ENVHANDLER_H