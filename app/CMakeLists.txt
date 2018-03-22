cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(../freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype EXCLUDE_FROM_ALL)
include_directories(../freetype/include)

add_library(native-lib SHARED
    src/main/cpp/native-lib.cpp)

target_link_libraries(native-lib
    android
    log
    GLESv2
    EGL
    freetype
    )
