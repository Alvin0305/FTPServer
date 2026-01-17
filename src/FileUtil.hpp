#ifndef FTPSERVER_FILEUTIL_H
#define FTPSERVER_FILEUTIL_H
#include <filesystem>

namespace fs = std::filesystem;

class FileUtil {
public:
    static fs::path getSandBoxedPath(const fs::path& path);
    static std::optional<fs::path> getResolvedPath(const fs::path &baseDir, const fs::path &currentPath, const std::string &userInput);
    static std::string listFilesAndFolders(const fs::path &folder);
    static std::string getStatOfFile(const std::filesystem::path &file);

private:
    static std::string getReadableSize(uintmax_t size);
};

#endif //FTPSERVER_FILEUTIL_H
