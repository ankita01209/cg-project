// =============================================================================
// Window.cpp — GLFW window + OpenGL 3.3 Core context implementation
// =============================================================================

#include "core/Window.h"

#include <stdexcept>
#include <cstdio>

namespace core {

// ── Construction / Destruction ────────────────────────────────────────────────

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    // ── GLFW initialisation ───────────────────────────────────────────────────
    // CG concept: GLFW is the cross-platform layer that creates a window
    // and binds an OpenGL "context" to it.  The context owns all GPU state.
    if (!glfwInit())
        throw std::runtime_error("[Window] glfwInit() failed.");

    // Request OpenGL 3.3 Core (no deprecated legacy API)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    m_window = glfwCreateWindow(m_width, m_height, title.c_str(), nullptr, nullptr);
    if (!m_window)
    {
        glfwTerminate();
        throw std::runtime_error(
            "[Window] glfwCreateWindow() failed. "
            "Ensure your GPU drivers support OpenGL 3.3+.");
    }

    glfwMakeContextCurrent(m_window);

    // ── GLAD — load all OpenGL function pointers ──────────────────────────────
    // CG concept: OpenGL functions are loaded at runtime via function pointers
    // because different GPUs/drivers expose different entry points.
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        throw std::runtime_error("[Window] gladLoadGLLoader() failed.");
    }

    printf("[Window] OpenGL %d.%d context ready.\n", GLVersion.major, GLVersion.minor);

    // ── Framebuffer size callback ─────────────────────────────────────────────
    // CG concept: the viewport maps NDC (–1..1) onto actual pixel coordinates.
    // When the window resizes we must update the viewport to match.
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Set initial viewport
    glViewport(0, 0, m_width, m_height);

    // Enable V-sync (swap interval 1) to prevent tearing and cap frame rate
    glfwSwapInterval(1);
}

Window::~Window()
{
    if (m_window)
        glfwDestroyWindow(m_window);
    glfwTerminate();
}

// ── Per-frame helpers ─────────────────────────────────────────────────────────

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() const
{
    // Processes pending OS events (keyboard, mouse, resize, close button, etc.)
    glfwPollEvents();
}

void Window::swapBuffers() const
{
    // CG concept – Double buffering:
    //   We draw to a hidden "back buffer" while the "front buffer" is displayed.
    //   Swapping them presents the completed frame without visible tearing.
    glfwSwapBuffers(m_window);
}

float Window::aspectRatio() const
{
    return (m_height > 0) ? static_cast<float>(m_width) / static_cast<float>(m_height) : 1.0f;
}

void Window::setTitle(const std::string& title)
{
    glfwSetWindowTitle(m_window, title.c_str());
}

// ── Static GLFW callback ──────────────────────────────────────────────────────

void Window::framebufferSizeCallback(GLFWwindow* win, int w, int h)
{
    // Retrieve our Window* stored in the GLFW user pointer
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
    self->m_width  = w;
    self->m_height = h;

    // Update the OpenGL viewport to the new framebuffer size
    glViewport(0, 0, w, h);
}

} // namespace core
