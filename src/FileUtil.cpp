#include "FileUtil.hpp"

#include <vector>

fs::path FileUtil::getSandBoxedPath(const std::filesystem::path &path) {
    return std::filesystem::relative(path, STORAGE_DIR);
}

std::optional<fs::path> FileUtil::getResolvedPath(const fs::path &baseDir, const fs::path &currentPath,
                                                  const std::string &userInput) {
    try {
        if (fs::path(userInput).is_absolute()) {
            return std::nullopt;
        }

        fs::path candidate = fs::weakly_canonical(currentPath / userInput);
        fs::path canonicalBase = fs::canonical(baseDir);

        auto baseIt = canonicalBase.begin();
        auto candIt = candidate.begin();

        for (; baseIt != canonicalBase.end(); ++baseIt, ++candIt) {
            if (candIt == candidate.end() || *baseIt != *candIt) {
                return std::nullopt;
            }
        }

        if (!fs::exists(candidate) || !fs::is_directory(candidate)) {
            return std::nullopt;
        }

        return candidate;
    } catch (...) {
        return std::nullopt;
    }
}

std::string FileUtil::listFilesAndFolders(const fs::path &folder) {
    std::string result;

    for (const auto &entry: fs::directory_iterator(folder)) {
        if (entry.is_directory()) {
            result.append("Dir: ").append(entry.path().filename().string()).append("\n");
        } else {
            result.append("File: ").append(entry.path().filename().string()).append("\n");
        }
    }

    return result;
}

std::string FileUtil::getStatOfFile(const std::filesystem::path &file) {
    std::ostringstream out;
    out << "Name        : " << file.filename().string() << std::endl;
    out << "Type        : " << (fs::is_regular_file(file) ? "FILE" : "DIRECTORY") << std::endl;
    if (fs::is_regular_file(file)) {
        out << "Size        : " << getReadableSize(fs::file_size(file)) << std::endl;
    }

    const fs::perms perms = fs::status(file).permissions();
    out << "Permissions : "
            << ((perms & fs::perms::owner_read) != fs::perms::none ? "r" : "-")
            << ((perms & fs::perms::owner_write) != fs::perms::none ? "w" : "-")
            << ((perms & fs::perms::owner_exec) != fs::perms::none ? "x" : "-");

    return out.str();
}

std::string FileUtil::getReadableSize(uintmax_t size) {
    const std::vector<std::string> units = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};

    int index = 0;
    while (size >= 1024 && index < units.size()) {
        size /= 1024;
        index++;
    }

    return std::to_string(size) + units[index];
}
