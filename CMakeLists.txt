cmake_minimum_required(VERSION 3.10)
project(ElementaryCLI)

add_library(ElementaryCLI src/cli.c src/line_buffer.c)
target_include_directories (ElementaryCLI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(demo exemple/demo_ncurses.c)
find_package(Curses REQUIRED)
target_include_directories(demo PUBLIC "${PROJECT_SOURCE_DIR}/src" ${CURSES_INCLUDE_DIR})
target_link_libraries(demo LINK_PUBLIC ElementaryCLI ${CURSES_LIBRARIES})
add_definitions(-DDEBUG)