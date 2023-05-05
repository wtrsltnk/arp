
CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY "glfw/glfw"
    GIT_TAG 3.3.4
    GIT_SHALLOW ON
    OPTIONS
        "GLFW_BUILD_EXAMPLES Off"
        "GLFW_BUILD_TESTS Off"
        "GLFW_BUILD_DOCS Off"
        "GLFW_INSTALL Off"
)

CPMAddPackage(
    NAME imgui
    GIT_TAG v1.82
    GITHUB_REPOSITORY ocornut/imgui
    DOWNLOAD_ONLY True
)

if (imgui_ADDED)
    add_library(imgui
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_demo.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    )

    target_include_directories(imgui
        PUBLIC
            "include"
            "${imgui_SOURCE_DIR}/"
    )

    target_link_libraries(imgui
        PRIVATE
            glfw
    )

    target_compile_options(imgui
        PUBLIC
            -DIMGUI_IMPL_OPENGL_LOADER_GLAD
    )
else()
    message(FATAL_ERROR "IMGUI not found")
endif()

CPMAddPackage(
    NAME RtMidi
    GITHUB_REPOSITORY "thestk/rtmidi"
    GIT_TAG 5.0.0
    DOWNLOAD_ONLY True
    OPTIONS
        "PA_BUILD_TESTS Off"
        "PA_BUILD_EXAMPLES Off"
)

add_library(RtMidi
    "${RtMidi_SOURCE_DIR}/RtMidi.cpp"
)

target_include_directories(RtMidi
    PUBLIC
        "${RtMidi_SOURCE_DIR}"
)

target_compile_definitions(RtMidi
    PUBLIC
        "-D__WINDOWS_MM__"
)
