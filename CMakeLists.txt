cmake_minimum_required(VERSION 3.5)

project(gb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${CMAKE_SOURCE_DIR}/src/*.h)

add_executable(gb ${SOURCE_FILES} ${HEADER_FILES})


if (UNIX)
    target_link_libraries(gb PRIVATE -lX11 -lGL -lpthread -lpng -lstdc++fs)
endif()


