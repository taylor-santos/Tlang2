cmake_minimum_required(VERSION 3.15)
project(compiler C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wextra")

find_package(BISON 3.5.0)
find_package(FLEX)

bison_target(
        Parser ${CMAKE_SOURCE_DIR}/src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c
)
flex_target(
        Scanner ${CMAKE_SOURCE_DIR}/src/scanner.l ${CMAKE_CURRENT_BINARY_DIR}/scanner.c
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scanner.h
)
add_flex_bison_dependency(Scanner Parser)

include_directories(${CMAKE_CURRENT_BINARY_DIR} include)

file(GLOB_RECURSE SRC "src/*.c")

add_executable(compiler
        ${BISON_Parser_OUTPUTS}
        ${FLEX_Scanner_OUTPUTS}
        ${SRC})