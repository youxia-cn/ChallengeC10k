cmake_minimum_required(VERSION 3.10)

project(ChallengeC10k)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server "server.cpp")
add_executable(client "client.cpp")
