cmake_minimum_required(VERSION 3.10)
project(SimpleDatabase)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler flags for debugging and optimization
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Create the executable
add_executable(simple_db 
    database.cpp
    database_engine.cpp
    query_parser.cpp
    table.cpp
)

# Link libraries if needed
target_link_libraries(simple_db)

# Enable testing
enable_testing()