if(MSYS AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_EXE_LINKER_FLAGS "-mwindows")
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SFML 2 COMPONENTS window system REQUIRED)
find_package(Freetype REQUIRED)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${SFML_INCLUDE_DIRS}
    )

add_executable(textogl_demo
    demo.cpp)

target_link_libraries(textogl_demo
    textogl
    ${FREETYPE_LIBRARIES}
    ${SFML_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    )
