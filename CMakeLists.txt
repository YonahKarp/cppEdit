cmake_minimum_required(VERSION 3.16)
project(justtype)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)

add_executable(justtype
    src/main.cpp
    src/app.cpp
    src/editor.cpp
    src/sidebar.cpp
    src/input.cpp
    src/search.cpp
    src/icons.cpp)

target_include_directories(justtype PRIVATE
    external
    ${SDL2_INCLUDE_DIRS})

target_link_libraries(justtype PRIVATE ${SDL2_LIBRARIES})
