cmake_minimum_required(VERSION 3.10)
project(WandBQueryOptimizer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add executable
add_executable(ml_query_optimizer ml_query_optimizer.cpp)

# Link math library
target_link_libraries(ml_query_optimizer m)

# Install target
install(TARGETS ml_query_optimizer DESTINATION bin)
