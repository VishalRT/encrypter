cmake_minimum_required(VERSION 3.31.5)

project(encrypter)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

# We are passing in build flags that are working. These aren't really required
# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")

if(CMAKE_BUILD_TYPE STREQUAL "")
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

# CLI11
add_subdirectory(external/CLI11)

# C++ Core Guidelines enforcement with clang-tidy
option(ENABLE_CLANG_TIDY "Enable clang-tidy naming checks" OFF)

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  set(CMAKE_CXX_CLANG_TIDY
      ${CLANG_TIDY_EXE})
endif()

# AddressSanitizer (ASan) option
option(ENABLE_ASAN "Enable AddressSanitizer (ASan)" OFF)

if(ENABLE_ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/password_prompt.cpp
  src/file_encryption.cpp
  src/file_watcher.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  kernel32 OpenSSL::Crypto
  CLI11::CLI11
)


#TODO: Include the below industry standard clang-tidy
# checks once we have the codebase in a good shape.
# We can enable them incrementally as we fix the issues.
# option(ENABLE_CLANG_TIDY_CHECKS "Enable clang-tidy" ON)
# if(ENABLE_CLANG_TIDY_CHECKS)
#     find_program(CLANG_TIDY_EXE clang-tidy)

#     if(CLANG_TIDY_EXE)
#         set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
#     endif()
# endif()

# TODO: Remove this once c++ 23 feature are fully available in clang
# https://www.reddit.com/r/cpp/comments/1b6f3s4/comment/ktcxhha/
# if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
#   message("Using gcc libc++ temporarily")
#   add_compile_options(
#     -stdlib=libstdc++ -D__cpp_concepts=202002 -Wno-builtin-macro-redefined
#   )
# endif()
