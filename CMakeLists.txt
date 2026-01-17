cmake_minimum_required(VERSION 4.1)
project(FTPServer)

set(CMAKE_CXX_STANDARD 23)

add_executable(FTPServer
        src/Server.cpp
        src/ClientHandler.cpp
        src/Parser.cpp
        src/FileUtil.cpp
        src/AuthHandler.cpp
)

add_executable(FTPClient
        src/Client.cpp
        src/Parser.cpp
        src/EnvHandler.cpp
)

target_compile_definitions(FTPServer
        PRIVATE STORAGE_DIR="${CMAKE_SOURCE_DIR}/storage"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

target_include_directories(FTPServer PRIVATE ${SODIUM_INCLUDE_DIRS})

find_package(Threads REQUIRED)
target_link_libraries(FTPServer
        PRIVATE Threads::Threads
        PRIVATE ${SODIUM_LIBRARIES}
)