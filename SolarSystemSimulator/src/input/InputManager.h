#pragma once
// =============================================================================
// InputManager.h — Handles GLFW input callbacks and camera tracking
//
// CG concept: Interaction
//   Translates raw device inputs (mouse/keyboard events) into high-level
//   simulation state changes, like camera orbiting and object focusing.
// =============================================================================

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace camera { class Camera; }
namespace scene { class SolarSystem; }
namespace core { class Window; }

namespace input {

class InputManager
{
public:
    InputManager(core::Window& window, camera::Camera& camera, scene::SolarSystem& solarSystem);
    ~InputManager();

    // ── Continuous tracking ───────────────────────────────────────────────────
    void update(float deltaTime);

    // ── Simulation controls (Phase 10 prep) ───────────────────────────────────
    bool  isPaused() const { return m_paused; }
    float getTimeScale() const { return m_timeScale; }

    int getFocusedBodyIndex() const { return m_focusedBodyIndex; }

    // ── Addition 2: Cinematic Tour Mode ───────────────────────────────────────
    enum class TourState { Off, Transitioning, Dwelling };
    TourState getTourState() const { return m_tourState; }
    int       getTourTargetIndex() const { return m_tourTargetIndex; }
    float     getTourTimer() const { return m_tourTimer; } // Addition 2: Timer access for UI Fades

private:
    core::Window&        m_window;
    camera::Camera&      m_camera;
    scene::SolarSystem&  m_solarSystem;

    // Camera control state
    bool   m_isDragging = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    
    // Focus tracking (-1 = overview, 0 = Sun, 1-8 = Planets)
    int    m_focusedBodyIndex = -1; 

    // Addition 2: Tour State Machine
    TourState m_tourState = TourState::Off;
    int       m_tourTargetIndex = 0;
    float     m_tourTimer = 0.0f;
    
    glm::vec3 m_tourStartTarget;
    float     m_tourStartDist = 5.0f;
    float     m_tourStartYaw  = 0.0f;
    float     m_tourStartPitch= 0.3f;

    void startTourTransition();
    void cancelTour();

    // Simulation state
    bool   m_paused = false;
    float  m_timeScale = 1.0f;

    // GLFW C-callbacks
    static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* win, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* win, double xoffset, double yoffset);
    static void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
};

} // namespace input
