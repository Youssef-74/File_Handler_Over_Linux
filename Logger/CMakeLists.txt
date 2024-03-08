cmake_minimum_required(VERSION 3.0)
project(Logger)

# Disable using Boost as static libraries
set(Boost_USE_STATIC_LIBS OFF)

# Enable multithreading support in Boost
set(Boost_USE_MULTITHREADED ON)

# Find Boost components required for the project
find_package(Boost 1.71 COMPONENTS log log_setup system thread REQUIRED)

# Include Boost headers
include_directories(${Boost_INCLUDE_DIR})

# Link against Boost libraries
link_directories(${Boost_LIBRARY_DIRS})

# Create shared library named "Logger" from the source file Logger.cpp
add_library(Logger Logger.cpp)

# Link the shared library against Boost libraries and pthread
target_link_libraries(Logger ${Boost_LIBRARIES} pthread)
