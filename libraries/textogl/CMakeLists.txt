cmake_minimum_required(VERSION 3.0)
project(textogl)

set(TITLE "Textogl")
set(AUTHOR "Matthew Chandler <tardarsauce@gmail.com>")
set(COPYRIGHT "2018 Matthew Chandler")
set(SUMMARY "OpenGL Text renderer")
set(WEBSITE "http://github.com/mattvchandler/${PROJECT_NAME}")

set(VERSION_MAJOR 1)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

set(VERSION_SHORT "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

if (CMAKE_VERSION VERSION_LESS "3.1")
    add_compile_options(-std=c++11)
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

add_compile_options(-Wall -Wextra)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")

# libraries
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules)

option(USE_GLM "Search for GLM and use that instead of internal Color / Vec2" ON)

add_subdirectory(src)

option(BUILD_DEMO "build a demo / test program" OFF)
if(BUILD_DEMO)
    add_subdirectory(demo)
endif()

# Doxygen documentation
option(TEXTOGL_GEN_DOCS "Generate 'doc' target" ON)
option(TEXTOGL_INTERNAL_DOCS "Generate documentation for private/internal methods, members, and functions" OFF)

if(TEXTOGL_GEN_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)

        file(GENERATE OUTPUT ${PROJECT_BINARY_DIR}/doxygen_gen.cmake CONTENT
            "set(PROJECT_NAME \"${PROJECT_NAME}\")
            set(TITLE \"${TITLE}\")
            set(SUMMARY \"${SUMMARY}\")
            set(VERSION_SHORT \"${VERSION_SHORT}\")
            set(CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\")
            set(CMAKE_CURRENT_SOURCE_DIR  \"${CMAKE_CURRENT_SOURCE_DIR}\")
            set(TEXTOGL_INTERNAL_DOCS ${TEXTOGL_INTERNAL_DOCS})

            if(TEXTOGL_INTERNAL_DOCS)
                set(TEXTOGL_EXTRACT_INTERNAL_DOCS \"YES\")
                set(TEXTOGL_NO_EXTRACT_INTERNAL_DOCS \"NO\")
                set(TEXTOGL_DOC_FILE_PATTERNS \"*.cpp *.hpp *.md\")
                set(TEXTOGL_DOC_ENABLED_SECTIONS \"INTERNAL\")
            else()
                set(TEXTOGL_EXTRACT_INTERNAL_DOCS \"NO\")
                set(TEXTOGL_NO_EXTRACT_INTERNAL_DOCS \"YES\")
                set(TEXTOGL_DOC_FILE_PATTERNS \"*.hpp *.md\")
                set(TEXTOGL_DOC_ENABLED_SECTIONS \"\")
            endif()

            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                ${PROJECT_BINARY_DIR}/Doxyfile)
            ")

        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/Doxyfile
            COMMAND ${CMAKE_COMMAND} -P ${PROJECT_BINARY_DIR}/doxygen_gen.cmake
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)

        add_custom_command(
            OUTPUT ${PROJECT_BINARY_DIR}/doc/html/index.html
            COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
            DEPENDS ${PROJECT_BINARY_DIR}/Doxyfile
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/README.md
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
            COMMENT "Generating documentation with Doxygen")

        add_custom_target(doc DEPENDS ${PROJECT_BINARY_DIR}/doc/html/index.html)

        set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES doc)
    endif()
endif()

# pkg-config and cmake 'find_package' files
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config-version.cmake.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc
    @ONLY)

# installation

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}" DESTINATION "include")

install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake" DESTINATION "lib/cmake/${PROJECT_NAME}")
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake" DESTINATION "lib/cmake/${PROJECT_NAME}")
install(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION "share/pkgconfig")

# CPack vars
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_CONTACT "${AUTHOR}")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY)
set(CPACK_PACKAGE_NAME "lib${PROJECT_NAME}-dev")
set(CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${TITLE}: ${SUMMARY}")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

set(CPACK_SOURCE_IGNORE_FILES
    ${PROJECT_BINARY_DIR}/*
    ${CMAKE_CURRENT_SOURCE_DIR}/.git/*
    ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}
    )

# Debian-specific settings
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_program(DPKG "dpkg")
    if(DPKG)
        set(CPACK_GENERATOR DEB;TGZ)
        execute_process(COMMAND ${DPKG} --print-architecture
            OUTPUT_STRIP_TRAILING_WHITESPACE
            OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE)
        if(NOT INCLUDE_SQLITE)
            set(CPACK_DEBIAN_PACKAGE_DEPENDS
                "libsqlite3-0")
        else()
            set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
        endif()
        set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
        string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_DEBIAN_PACKAGE_NAME)
        set(CPACK_PACKAGE_FILE_NAME
            "${CPACK_DEBIAN_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
    endif()
endif()

include(CPack)
