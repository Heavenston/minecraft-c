cmake_minimum_required(VERSION 4.0)
project(mcc LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion")
set(CMAKE_C_FLAGS_DEBUG "-g3 -O1")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.c")

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

find_library(XCB NAMES xcb libxcb Xcb REQUIRED)
find_library(XCB_ICCCM NAMES xcb-icccm libxcb-icccm REQUIRED)
find_library(XCB_FIXES NAMES xcb-xfixes libxcb-xfixes REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB})
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB_ICCCM})
target_link_libraries(${PROJECT_NAME} PRIVATE ${XCB_FIXES})

# Link against the math library
target_link_libraries(${PROJECT_NAME} PRIVATE m)
