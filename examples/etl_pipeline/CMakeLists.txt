cmake_minimum_required(VERSION 3.16)
project(etl_pipeline)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Download nlohmann/json if not available
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json.hpp")
    message(STATUS "Downloading nlohmann/json...")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann")
    file(DOWNLOAD 
        "https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nlohmann/json.hpp"
        SHOW_PROGRESS
    )
endif()

# Source files
set(SOURCES
    main.cpp
    sources/web_scraper.cpp
    sources/api_client.cpp
    sources/s3_client_simple.cpp
    sources/sftp_client_simple.cpp
    processors/data_transformer.cpp
    loaders/file_writer.cpp
)

# Create executable
add_executable(etl_pipeline ${SOURCES})

# Link libraries
target_link_libraries(etl_pipeline 
    CURL::libcurl
    stdc++fs
)

# Include nlohmann/json
target_include_directories(etl_pipeline PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Compiler flags
target_compile_options(etl_pipeline PRIVATE -Wall -Wextra)

# Install target
install(TARGETS etl_pipeline DESTINATION bin)