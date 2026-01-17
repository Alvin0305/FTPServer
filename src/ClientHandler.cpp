#include "ClientHandler.hpp"

#include <cinttypes>
#include <fstream>
#include <iostream>
#include <optional>
#include <unistd.h>
#include <sys/socket.h>

#include "AuthHandler.hpp"
#include "Parser.hpp"
#include "constants.hpp"
#include "FileUtil.hpp"
#include "Util.hpp"

ClientHandler::ClientHandler(const int clientSocket) : clientSocket(clientSocket) {
    authenticated = false;
    paired = false;
    currentPath = STORAGE_DIR;
    username = "";
}

Request ClientHandler::readRequest() {
    ssize_t n;
    char buffer[BUFFER_SIZE];

    size_t pos;
    if ((pos = recvBuffer.find('\n')) != std::string::npos) {
        const std::string requestString = recvBuffer.substr(0, pos);

        recvBuffer.erase(0, pos + 1);
        std::cout << "Received request: " << requestString << std::endl;
        return Parser::parseRequest(requestString);
    }

    while ((n = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
        recvBuffer.append(buffer, n);

        if ((pos = recvBuffer.find('\n')) != std::string::npos) {
            const std::string requestString = recvBuffer.substr(0, pos);

            recvBuffer.erase(0, pos + 1);
            std::cout << "Received request: " << requestString << std::endl;
            return Parser::parseRequest(requestString);
        }
    }
    return {};
}

void ClientHandler::sendResponse(const StatusCode statusCode, const std::string &responseType,
                                 const std::string &data) const {
    const std::string content = std::to_string(statusCode) + " " + responseType + " " + std::to_string(data.length()) +
                                "\n" + data + "\n" + "END\n";

    if (send(clientSocket, content.c_str(), content.length(), MSG_NOSIGNAL) <= 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }
}

void ClientHandler::sendFile(const fs::path &path, const std::string &requestType) const {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << path << std::endl;
        return;
    }

    const uintmax_t size = fs::file_size(path);
    uintmax_t totalReceived = 0;
    const std::string header = std::to_string(OK).append(" ").append(requestType).append(" ").append(
        std::to_string(size)).append(" ").append(path.filename()).append("\n");

    if (send(clientSocket, header.c_str(), header.length(), MSG_NOSIGNAL) <= 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    while (file.read(buffer, 1024) || file.gcount() > 0) {
        const std::streamsize bytesRead = file.gcount();
        totalReceived += bytesRead;

        Util::showProgressBar(totalReceived, size);

        if (bytesRead > 0) {
            if (send(clientSocket, buffer, bytesRead, MSG_NOSIGNAL) <= 0) {
                std::cerr << "Error sending file data over socket " << path << std::endl;
                file.close();
                return;
            }
        }
    }

    file.close();
    std::string end = "\nEND\n";
    if (send(clientSocket, end.c_str(), end.length(), MSG_NOSIGNAL) <= 0) {
        std::cerr << "failed to send END" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << std::endl;
}

bool ClientHandler::saveFileToTemp(const Request &request) {
    char *end = nullptr;
    const uintmax_t fileSize = std::strtoumax(request.args[1].c_str(), &end, 10);
    uintmax_t totalReceived = 0;
    if (fileSize <= 0) {
        return false;
    }

    std::filesystem::path storageDir = STORAGE_DIR;
    std::ofstream file(storageDir / (request.args[0] + ".tmp"), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << (request.args[0] + ".tmp") << std::endl;
        return false;
    }

    if (!recvBuffer.empty()) {
        file.write(recvBuffer.c_str(), static_cast<std::streamsize>(recvBuffer.length()));
        totalReceived += recvBuffer.length();
        recvBuffer.erase();
    }

    char buffer[BUFFER_SIZE];
    while (totalReceived < fileSize) {
        const ssize_t received = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        const uintmax_t needed = fileSize - totalReceived;

        if (received <= needed) {
            file.write(buffer, received);
            totalReceived += received;
        } else {
            file.write(buffer, static_cast<std::streamsize>(needed));
            recvBuffer.append(buffer + needed, received - needed);
            totalReceived += received;
        }
        Util::showProgressBar(totalReceived, fileSize);
    }

    file.close();
    std::cout << std::endl;
    return true;
}

ClientState ClientHandler::commandHandler(const Request &request) {
    if (request.command == "PING") {
        sendResponse(OK, "PING", "PONG");
    } else if (request.command == "QUIT") {
        sendResponse(OK, "QUIT", "");
        return STOP;
    } else if (request.command == "HELP") {
        sendResponse(
            OK,
            "HELP",
            "Available Commands:\n"
            "\n"
            "Connection & Session:\n"
            "  PING                          Check server connection\n"
            "  HELP                          Show this help message\n"
            "  QUIT                          Disconnect from server\n"
            "\n"
            "Authentication:\n"
            "  LOGIN <username> <password>   Login to the server\n"
            "  LOGOUT                        Logout from current session\n"
            "  PASSWD <new_password>         Change account password\n"
            "\n"
            "Navigation:\n"
            "  PWD                           Show current directory\n"
            "  LS                            List files and folders\n"
            "  CD <folder>                   Change directory\n"
            "\n"
            "File Operations:\n"
            "  GET <file>                    Download a file\n"
            "  PUT <file>                    Upload a file\n"
            "  CAT <file>                    Display file contents\n"
            "  RM <file>                     Delete a file\n"
            "  RENAME <old> <new>            Rename file or folder\n"
            "  CP <src> <dest>               Copy file or folder\n"
            "  MV <src> <dest>               Move file or folder\n"
            "  STAT <file>                   View Details of the file\n"
            "\n"
            "Directory Operations:\n"
            "  MKDIR <folder>                Create a directory\n"
            "  RMDIR <folder>                Delete a directory\n"
            "\n"
            "Storage Location:\n"
            "  SDD                           Show storage directory\n"
            "  PDD <folder>                  Set storage directory\n"
            "\n"
            "Notes:\n"
            "  - Most commands require login\n"
            "  - Paths are restricted to your storage directory\n"
        );
    } else if (request.command == "LOGIN") {
        if (authenticated || paired) {
            sendResponse(BAD_REQUEST, "LOGIN", "Already Logged In");
        } else if (request.args.size() != 2) {
            sendResponse(BAD_REQUEST, "LOGIN", "Invalid Format, login <username> <password>");
        } else if (AuthHandler::isAuthenticated(request.args[0], request.args[1])) {
            authenticated = true;
            currentPath = STORAGE_DIR;
            username = request.args[0];
            sendResponse(OK, "LOGIN", "Logged In");
        } else {
            sendResponse(NOT_AUTHORIZED, "LOGIN", "Invalid Credentials");
        }
    } else if (request.command == "LOGOUT") {
        if (!authenticated && !paired) {
            sendResponse(BAD_REQUEST, "LOGOUT", "Already Logged Out");
        } else {
            authenticated = false;
            username = "";
            sendResponse(OK, "LOGOUT", "Successfully Logged Out");
        }
    } else if (request.command == "PWD") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "PWD", "Login Required");
        } else {
            sendResponse(OK, "PWD", FileUtil::getSandBoxedPath(currentPath));
        }
    } else if (request.command == "CD") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "CD", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(NOT_AUTHORIZED, "CD", "Invalid Format, cd <foldername>");
        } else if (!fs::exists(currentPath / request.args[0])) {
            sendResponse(NOT_FOUND, "CD", "No such folder");
        } else if (!fs::is_directory(currentPath / request.args[0])) {
            sendResponse(BAD_REQUEST, "CD", "is not a folder");
        } else {
            const auto newPath = FileUtil::getResolvedPath(STORAGE_DIR, currentPath, request.args[0]);
            if (!newPath) {
                sendResponse(NOT_AUTHORIZED, "CD", "Not authorized");
            } else {
                currentPath = *newPath;
                sendResponse(OK, "CD", "Directory Changed");
            }
        }
    } else if (request.command == "LS") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "LS", "Login Required");
        } else {
            sendResponse(OK, "LS", FileUtil::listFilesAndFolders(currentPath));
        }
    } else if (request.command == "GET") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "GET", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(BAD_REQUEST, "GET", "Invalid Format, get <filename>");
        } else if (fs::is_directory((currentPath / request.args[0]))) {
            sendResponse(BAD_REQUEST, "GET", "Cannot get a folder, get a file");
        } else if (!fs::exists(currentPath / request.args[0])) {
            sendResponse(NOT_FOUND, "GET", "No such file");
        } else {
            sendFile(currentPath / request.args[0], "GET");
        }
    } else if (request.command == "MKDIR") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "MKDIR", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(NOT_AUTHORIZED, "MKDIR", "Invalid Format, mkdir <foldername>");
        } else if (std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "MKDIR", "Folder already exists");
        } else {
            std::filesystem::create_directory(currentPath / request.args[0]);
            sendResponse(OK, "MKDIR", "Directory Created");
        }
    } else if (request.command == "RMDIR") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "RMDIR", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(NOT_AUTHORIZED, "RMDIR", "Invalid Format, rmdir <foldername>");
        } else if (!std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "RMDIR", "Folder does not exist");
        } else if (!std::filesystem::is_directory(currentPath / request.args[0])) {
            sendResponse(BAD_REQUEST, "RMDIR", "is a file");
        } else {
            std::filesystem::remove_all(currentPath / request.args[0]);
            sendResponse(OK, "RMDIR", "Directory Deleted");
        }
    } else if (request.command == "RM") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "RM", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(NOT_AUTHORIZED, "RM", "Invalid Format, rm <foldername>");
        } else if (!std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "RM", "File does not exist");
        } else if (std::filesystem::is_directory(currentPath / request.args[0])) {
            sendResponse(BAD_REQUEST, "RM", "is a folder");
        } else {
            std::filesystem::remove(currentPath / request.args[0]);
            sendResponse(OK, "RM", "File Deleted");
        }
    } else if (request.command == "RENAME") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "RENAME", "Login Required");
        } else if (request.args.size() != 2) {
            sendResponse(NOT_AUTHORIZED, "RENAME", "Invalid Format, rename <oldname> <newname>");
        } else if (request.args[1].contains(' ')) {
            sendResponse(BAD_REQUEST, "RENAME", "File names cannot contain space");
        } else if (!std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "RENAME", "File / Folder does not exist");
        } else if (std::filesystem::exists(currentPath / request.args[1])) {
            sendResponse(FORBIDDEN, "RENAME", "New name already not exist");
        } else {
            std::filesystem::rename(currentPath / request.args[0], currentPath / request.args[1]);
            sendResponse(OK, "RENAME", "File Renamed");
        }
    } else if (request.command == "CP") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "CP", "Login Required");
        } else if (request.args.size() != 2) {
            sendResponse(NOT_AUTHORIZED, "CP", "Invalid Format, cp <src> <dest>");
        } else if (!std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "CP", "File / Folder does not exist");
        } else if (std::filesystem::exists(currentPath / request.args[1])) {
            sendResponse(FORBIDDEN, "CP", "Destination file already exists");
        } else {
            std::filesystem::copy(currentPath / request.args[0], currentPath / request.args[1],
                                  std::filesystem::copy_options::recursive);
            sendResponse(OK, "CP", "File Copied");
        }
    } else if (request.command == "MV") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "MV", "Login Required");
        } else if (request.args.size() != 2) {
            sendResponse(NOT_AUTHORIZED, "MV", "Invalid Format, mv <src> <dest>");
        } else if (!std::filesystem::exists(currentPath / request.args[0])) {
            sendResponse(FORBIDDEN, "MV", "File / Folder does not exist");
        } else if (std::filesystem::exists(currentPath / request.args[1])) {
            sendResponse(FORBIDDEN, "MV", "Destination file already exists");
        } else {
            std::filesystem::rename(currentPath / request.args[0], currentPath / request.args[1]);
            sendResponse(OK, "MV", "File Moved");
        }
    } else if (request.command == "PASSWD") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "PASSWD", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(BAD_REQUEST, "PASSWD", "Invalid Format, passwd <new>");
        } else {
            AuthHandler::updatePassword(username, request.args[0]);
            sendResponse(OK, "PASSWD", "Password Updated");
        }
    } else if (request.command == "PUT") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "PUT", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(BAD_REQUEST, "PUT", "Invalid Format, put <filename>");
        } else {
            sendResponse(OK, "PUT", "Ready to receive");
        }
    } else if (request.command == "PUT_") {
        const bool result = saveFileToTemp(request);
        if (!result) {
            sendResponse(SERVER_ERROR, "PUT", "Upload failed");
        } else {
            std::filesystem::path savedTemp = STORAGE_DIR;
            savedTemp /= request.args[0] + ".tmp";

            std::filesystem::rename(savedTemp, currentPath / request.args[0]);
            sendResponse(OK, "PUT", "File Uploaded");
        }
    } else if (request.command == "CAT") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "CAT", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(BAD_REQUEST, "CAT", "Invalid Format, cat <filename>");
        } else if (fs::is_directory((currentPath / request.args[0]))) {
            sendResponse(BAD_REQUEST, "CAT", "Cannot cat a folder, cat a file");
        } else if (!fs::exists(currentPath / request.args[0])) {
            sendResponse(NOT_FOUND, "CAT", "No such file");
        } else {
            sendFile(currentPath / request.args[0], "CAT");
        }
    } else if (request.command == "STAT") {
        if (!authenticated && !paired) {
            sendResponse(NOT_AUTHORIZED, "STAT", "Login Required");
        } else if (request.args.size() != 1) {
            sendResponse(BAD_REQUEST, "STAT", "Invalid Format, stat <filename>");
        } else if (fs::is_directory((currentPath / request.args[0]))) {
            sendResponse(BAD_REQUEST, "STAT", "Cannot stat a folder, stat a file");
        } else if (!fs::exists(currentPath / request.args[0])) {
            sendResponse(NOT_FOUND, "STAT", "No such file");
        } else {
            const std::filesystem::path file = currentPath / request.args[0];
            sendResponse(OK, "STAT", FileUtil::getStatOfFile(file));
        }
    } else {
        sendResponse(BAD_REQUEST, "ERROR", "Unknown Command: " + request.command);
    }

    return CONTINUE;
}

void ClientHandler::run() {
    while (true) {
        Request request = readRequest();

        if (commandHandler(request) == STOP) {
            break;
        }
    }

    std::cout << "Closing Connection with Client " << clientSocket << std::endl;
    close(clientSocket);
}
