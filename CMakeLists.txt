cmake_minimum_required(VERSION 3.31.5)

project(encrypter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# We are passing in build flags that are working. These aren't really required
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")

#OPENSSL
##Helpers to find openssl and libs
if(DEFINED ENV{OPENSSL_ROOT_DIR})
    message(STATUS "Openssl root provided $ENV{OPENSSL_ROOT_DIR}")
    set(OPENSSL_ROOT_DIR "$ENV{OPENSSL_ROOT_DIR}")
else()
    message(WARNING "Openssl root not provided in env variable")
endif()

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL 3.4 REQUIRED)
# Uncomment to Debug OpenSSL with CMake
message(STATUS "OPENSSL FOUND ${OPENSSL_FOUND}")
# message("Openssl Include ${OPENSSL_INCLUDE_DIR}")
# message("Openssl version ${OPENSSL_VERSION}")
# message("Openssl Libraries: ${OPENSSL_LIBRARIES}")

#Windows, OpenSSL API inclusion
# add_compile_options("$ENV{CFLAGS}" "$ENV{CXXFLAGS}")
# link_directories("$ENV{LDFLAGS}")

# Add OpenSSL include directory globally
include_directories(${OPENSSL_INCLUDE_DIR})
# Also add explicit compile flags to ensure includes are passed to clangd
add_compile_options(-I${OPENSSL_INCLUDE_DIR})

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE kernel32 OpenSSL::Crypto)

# C++ Core Guidelines enforcement with clang-tidy
option(ENABLE_CLANG_TIDY_CHECKS "Enable clang-tidy naming checks" OFF)

if(ENABLE_CLANG_TIDY_CHECKS)
    find_program(CLANG_TIDY clang-tidy)
    if(CLANG_TIDY)
        message(STATUS "clang-tidy found: ${CLANG_TIDY}")
        # Create custom target for clang-tidy checks
        add_custom_target(clang-tidy-check
            COMMAND ${CLANG_TIDY}
                -checks="-*,readability-identifier-naming"
                -p=${CMAKE_BINARY_DIR}
                ${CMAKE_SOURCE_DIR}/src/main.cpp
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running clang-tidy naming checks"
        )
    else()
        message(WARNING "clang-tidy not found - naming checks not available in build")
    endif()
endif()

# TODO: Remove this once c++ 23 feature are fully available in clang
# https://www.reddit.com/r/cpp/comments/1b6f3s4/comment/ktcxhha/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   message("Using gcc libc++ temporarily")
#   add_compile_options(
#     -stdlib=libstdc++ -D__cpp_concepts=202002 -Wno-builtin-macro-redefined
#   )
# endif()