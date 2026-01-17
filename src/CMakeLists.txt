cmake_minimum_required(VERSION 3.16)
project(FTPClient)

set(CMAKE_CXX_STANDARD 23)

add_executable(FTPClient
        Client.cpp
        Parser.cpp
        EnvHandler.cpp
)