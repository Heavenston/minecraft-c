cmake_minimum_required(VERSION 4.0)
project(mcc LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

find_library(XCB NAMES xcb libxcb Xcb REQUIRED)
find_library(XCB_ICCCM NAMES xcb-icccm libxcb-icccm REQUIRED)
find_library(XCB_FIXES NAMES xcb-xfixes libxcb-xfixes REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB})
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB_ICCCM})
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB_FIXES})

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wsign-conversion
    )
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()
