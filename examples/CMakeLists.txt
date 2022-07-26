cmake_minimum_required(VERSION 3.16.0)

project(SimpleBLE_Examples)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Nice hack to automatically ignore the build directory
file(WRITE ${CMAKE_BINARY_DIR}/.gitignore "*")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include SimpleBLE
# Build artifacts in a separate folder
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_BINARY_DIR}/simpleble)
include_directories(${SIMPLEBLE_INCLUDES})

# C++
add_subdirectory(cpp/list_adapters)
add_subdirectory(cpp/scan)
add_subdirectory(cpp/connect)
add_subdirectory(cpp/connect_safe)
add_subdirectory(cpp/read)
add_subdirectory(cpp/write)
add_subdirectory(cpp/notify)
add_subdirectory(cpp/notify_multi)
add_subdirectory(cpp/switchbot)
