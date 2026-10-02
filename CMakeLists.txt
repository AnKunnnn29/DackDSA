cmake_minimum_required(VERSION 3.16)
project(DackDSA LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# main.cpp includes OrderManager.cpp, which includes ProductLookup.cpp.
# Compile only main.cpp to avoid duplicate definitions.
add_executable(dsa_demo main.cpp)
if(MSVC)
  target_compile_options(dsa_demo PRIVATE /W4 /utf-8)
else()
  target_compile_options(dsa_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
