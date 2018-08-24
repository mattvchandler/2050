if(ANDROID) # TODO: could probably include iOS or other OpenGL ES platforms
    # it is up to the android project to make sure these are included and linked correctly
    set(OPENGL_LIBRARIES GLESv2)
    set(FREETYPE_LIBRARIES freetype)

    set(VERT_SHADER_SRC ${CMAKE_CURRENT_LIST_DIR}/shaders/font.gles20.vert)
    set(FRAG_SHADER_SRC ${CMAKE_CURRENT_LIST_DIR}/shaders/font.gles20.frag)
else()
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(Freetype REQUIRED)

    if(USE_GLM)
        find_package(GLM)
    endif()
    set(VERT_SHADER_SRC ${CMAKE_CURRENT_LIST_DIR}/shaders/font.gl33.vert)
    set(FRAG_SHADER_SRC ${CMAKE_CURRENT_LIST_DIR}/shaders/font.gl33.frag)
endif()

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_BINARY_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    )

add_library(${PROJECT_NAME}
    font.cpp
    font_common.cpp
    static_text.cpp
    )

if(GLM_FOUND)
    message(STATUS "Found glm, building with glm support")
    target_compile_definitions(${PROJECT_NAME} PUBLIC "-DUSE_GLM")
endif()

if(ANDROID)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "-DUSE_OPENGL_ES")
endif()

target_link_libraries(${PROJECT_NAME}
    ${FREETYPE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    )

# load the shader source code into C++ strings
file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/shaders.cmake CONTENT "
    file(READ ${VERT_SHADER_SRC} VERT_SHADER)
    file(READ ${FRAG_SHADER_SRC} FRAG_SHADER)
    configure_file(${CMAKE_CURRENT_LIST_DIR}/shaders/shaders.inl.in
        ${PROJECT_BINARY_DIR}/shaders.inl)
   ")

add_custom_command(
    COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/shaders.cmake
    DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/shaders/shaders.inl.in
        ${VERT_SHADER_SRC}
        ${FRAG_SHADER_SRC}
    OUTPUT
        ${PROJECT_BINARY_DIR}/shaders.inl
    COMMENT "Including shader source files"
    )

add_custom_target(shaders DEPENDS ${PROJECT_BINARY_DIR}/shaders.inl)

add_dependencies(${PROJECT_NAME} shaders)

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION "lib"
    LIBRARY DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    )
