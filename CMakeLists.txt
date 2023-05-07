cmake_minimum_required(VERSION 3.12)

project(arp VERSION "0.1.0")

include(cmake/CPM.cmake)
include(cmake/Dependencies.cmake)

configure_file(config.h.in config.h)

add_executable(arp
    include/app.hpp
    src/app-infra.cpp
    src/app.cpp
    src/glad.c
    src/program.cpp
    src/imgui_knob.cpp
)

target_compile_features(arp
    PRIVATE
        cxx_nullptr
)

target_include_directories(arp
    PRIVATE
        "include"
        "${PROJECT_BINARY_DIR}"
        "${PROJECT_BINARY_DIR}"
)

target_link_libraries(arp
    PRIVATE
        glfw
        imgui
        RtMidi
        winmm
)


