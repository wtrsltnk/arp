cmake_minimum_required(VERSION 3.12)

project(imgui-keys VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(imgui-keys
    include/app.hpp
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/program.cpp
)

target_compile_features(imgui-keys
    PRIVATE
        cxx_nullptr
)

target_include_directories(imgui-keys
    PRIVATE
        "include"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(imgui-keys
    PRIVATE
        glfw
        imgui
)


