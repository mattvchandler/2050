cmake_minimum_required(VERSION 3.4.1)

include_directories(
    ../freetype/include
    ../glm/
    ../textogl/include
    )

set(GLM_FOUND ON)

add_subdirectory(../freetype ${CMAKE_CURRENT_BINARY_DIR}/freetype EXCLUDE_FROM_ALL)
add_subdirectory(../textogl ${CMAKE_CURRENT_BINARY_DIR}/textogl EXCLUDE_FROM_ALL)

add_library(native-lib SHARED
    src/main/cpp/engine.cpp
    src/main/cpp/jni.cpp
    src/main/cpp/opengl.cpp
    src/main/cpp/world.cpp
    )

target_link_libraries(native-lib
    android
    log
    GLESv2
    EGL
    textogl
    )
