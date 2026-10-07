#pragma once
// =============================================================================
// Window.h — GLFW window + OpenGL 3.3 Core context wrapper
//
// CG concept: The "window" is also the OpenGL rendering surface.  GLFW gives
// us a platform-independent way to create that surface and the context that
// owns all GPU state.
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

namespace core {

class Window
{
public:
    // Creates a window with the requested dimensions and title.
    // Throws std::runtime_error on failure.
    Window(int width, int height, const std::string& title);
    ~Window();

    // Non-copyable (GLFW windows cannot be duplicated)
    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    // ── Per-frame helpers ─────────────────────────────────────────────────────
    bool shouldClose()  const;   ///< Returns true when the user closes the window
    void pollEvents()   const;   ///< Processes keyboard, mouse, window events
    void swapBuffers()  const;   ///< Flip front/back buffers (double-buffering)

    // ── Accessors ─────────────────────────────────────────────────────────────
    int            width()  const { return m_width;  }
    int            height() const { return m_height; }
    GLFWwindow*    handle() const { return m_window; }
    float          aspectRatio() const;

    // ── Convenience ───────────────────────────────────────────────────────────
    void setTitle(const std::string& title);

private:
    GLFWwindow* m_window = nullptr;
    int         m_width  = 0;
    int         m_height = 0;

    // GLFW callback — called whenever the framebuffer is resized
    static void framebufferSizeCallback(GLFWwindow* win, int w, int h);
};

} // namespace core
