# Init Project, Set C++ Standard
cmake_minimum_required(VERSION 3.16)
project(GCTRealMate VERSION 0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Added Compile Command Export for Intellisense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Init Executable
add_executable(${PROJECT_NAME} src/Code.cpp src/compileGCT.cpp src/GCTRealMate.cpp src/PPCop.cpp src/utility.cpp)
# Explicitly Link Include Directory
target_include_directories(${PROJECT_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include" )
