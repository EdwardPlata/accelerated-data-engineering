cmake_minimum_required(VERSION 3.10)
project(CompressionExample)

# Find zlib library
find_package(ZLIB REQUIRED)

# Add the source file
add_executable(compression compression.cpp)

# Link zlib
target_link_libraries(compression ZLIB::ZLIB)