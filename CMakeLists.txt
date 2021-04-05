cmake_minimum_required(VERSION 3.10)
project (ElementaryCLI)

add_library (ElementaryCLI src/cli.c)
target_include_directories (ElementaryCLI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable (cliExemple exemple/main.c)
target_include_directories(cliExemple PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries (cliExemple LINK_PUBLIC ElementaryCLI)