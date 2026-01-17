#include "FileUtil.hpp"

fs::path FileUtil::getSandBoxedPath(const std::filesystem::path& path) {
    return std::filesystem::relative(path, STORAGE_DIR);
}

std::optional<fs::path> FileUtil::getResolvedPath(const fs::path &baseDir, const fs::path &currentPath, const std::string &userInput) {
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

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_directory()) {
            result.append("Dir: ").append(entry.path().filename().string()).append("\n");
        } else {
            result.append("File: ").append(entry.path().filename().string()).append("\n");
        }
    }

    return result;
}
