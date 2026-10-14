cmake_minimum_required(VERSION 3.20)
project(SolarSystemSimulator CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── FetchContent ─────────────────────────────────────────────────────────────
include(FetchContent)

# GLFW – windowing + OpenGL context
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM – header-only math library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

# ImGui – UI library
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# ─── GLAD (vendored) ──────────────────────────────────────────────────────────
# Pre-generated for OpenGL 3.3 Core profile.
# Source: https://glad.dav1d.de (gl=Version 3.3, profile=Core, no extensions)
# Files live in libs/glad/ and are compiled once as a static library.
add_library(glad STATIC
    libs/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    libs/glad/include
)
# glad.c uses dlopen on Linux/macOS; on Windows opengl32.lib is sufficient.
if(WIN32)
    target_link_libraries(glad PUBLIC opengl32)
else()
    target_link_libraries(glad PUBLIC dl)
endif()

# ─── Main Executable ─────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/core/Window.cpp
    src/core/Shader.cpp
    src/camera/Camera.cpp
    src/render/Mesh.cpp
    src/render/Sphere.cpp
    src/render/Texture.cpp
    src/render/Bloom.cpp
    libs/stb/stb_image.cpp
    src/scene/SolarSystem.cpp
    src/scene/CelestialBody.cpp
    src/scene/AsteroidBelt.cpp
    src/input/InputManager.cpp
    src/ui/UIOverlay.cpp

    # ImGui sources
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    libs/glad/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# ASSETS_DIR is used in main.cpp to locate shader files at runtime.
# We store the absolute source-tree path so the executable always finds
# shaders regardless of the working directory it is launched from.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/"
    GLFW_INCLUDE_NONE
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glad
    glfw
    glm::glm
)
