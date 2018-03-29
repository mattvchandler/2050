cmake_minimum_required(VERSION 3.4.1)

include_directories(
    ../libraries/freetype/include
    ../libraries/glm/
    ../libraries/json/include/
    ../libraries/textogl/include
    )

set(GLM_FOUND ON)

add_subdirectory(../libraries/freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype EXCLUDE_FROM_ALL)
add_subdirectory(../libraries/textogl ${CMAKE_CURRENT_BINARY_DIR}/textogl EXCLUDE_FROM_ALL)

add_library(2050 SHARED
    src/main/cpp/ball.cpp
    src/main/cpp/engine.cpp
    src/main/cpp/jni.cpp
    src/main/cpp/opengl.cpp
    src/main/cpp/world.cpp
    )

target_link_libraries(2050
    android
    log
    GLESv2
    EGL
    textogl
    )
