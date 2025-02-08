cmake_minimum_required(VERSION 3.31.5)

project(encrypter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")


add_executable(${PROJECT_NAME} src/main.cpp)
