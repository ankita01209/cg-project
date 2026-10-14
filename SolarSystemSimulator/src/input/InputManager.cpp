// =============================================================================
// InputManager.cpp
// =============================================================================

#include "input/InputManager.h"
#include "camera/Camera.h"
#include "scene/SolarSystem.h"
#include "core/Window.h"

#include <algorithm> // for clamp

namespace input {

// Single active instance for static callbacks routing
static InputManager* s_instance = nullptr;

InputManager::InputManager(core::Window& window, camera::Camera& camera, scene::SolarSystem& solarSystem)
    : m_window(window), m_camera(camera), m_solarSystem(solarSystem)
{
    s_instance = this;

    GLFWwindow* win = m_window.handle();
    glfwSetMouseButtonCallback(win, mouseButtonCallback);
    glfwSetCursorPosCallback(win, cursorPosCallback);
    glfwSetScrollCallback(win, scrollCallback);
    glfwSetKeyCallback(win, keyCallback);
}

InputManager::~InputManager()
{
    if (s_instance == this)
        s_instance = nullptr;
}

void InputManager::startTourTransition()
{
    m_tourState = TourState::Transitioning;
    m_tourTimer = 0.0f;
    m_tourStartTarget = m_camera.getTarget();
    m_tourStartDist   = m_camera.getDistance();
    m_tourStartYaw    = m_camera.getYaw();
    m_tourStartPitch  = m_camera.getPitch();
}

void InputManager::cancelTour()
{
    m_tourState = TourState::Off;
}

void InputManager::update(float deltaTime)
{
    // ── Addition 2: Cinematic Tour Mode ───────────────────────────────────────
    if (m_tourState != TourState::Off)
    {
        const auto& bodies = m_solarSystem.getBodies();
        if (m_tourTargetIndex >= static_cast<int>(bodies.size()))
            m_tourTargetIndex = 0; // fallback safety
            
        const auto& targetBody = bodies[m_tourTargetIndex];
        
        // Good framing stats for this body
        float targetDistance = targetBody.radius * (m_tourTargetIndex == 0 ? 6.0f : 3.5f);
        float targetYaw      = 0.0f; // Could be anything, 0.0 is nice
        float targetPitch    = 0.4f; // Look slightly down
        
        if (m_tourState == TourState::Transitioning)
        {
            m_tourTimer += deltaTime;
            const float transitionDuration = 2.0f;
            
            float t = std::clamp(m_tourTimer / transitionDuration, 0.0f, 1.0f);
            
            // CG concept: Interpolation / Easing
            // A smoothstep function eases-in and eases-out, making camera moves
            // feel deliberate and cinematic rather than linear and robotic.
            float easedT = t * t * (3.0f - 2.0f * t);
            
            m_camera.setTarget(glm::mix(m_tourStartTarget, targetBody.position, easedT));
            m_camera.setDistance(glm::mix(m_tourStartDist, targetDistance, easedT));
            
            // Easing the angles
            m_camera.setYaw(glm::mix(m_tourStartYaw, targetYaw, easedT));
            m_camera.setPitch(glm::mix(m_tourStartPitch, targetPitch, easedT));
            
            if (t >= 1.0f)
            {
                m_tourState = TourState::Dwelling;
                m_tourTimer = 0.0f;
            }
        }
        else if (m_tourState == TourState::Dwelling)
        {
            m_tourTimer += deltaTime;
            const float dwellDuration = 4.8f; // Session 3 (Fix 3): Syncs with 3 Fun Facts x 1.6s each
            
            // Still track the body as it orbits (crucial for fast orbital speeds)
            m_camera.setTarget(targetBody.position);
            
            if (m_tourTimer >= dwellDuration)
            {
                m_tourTargetIndex++;
                if (m_tourTargetIndex >= 9) // We have sun (0) + 8 planets = 9 bodies
                {
                    cancelTour();
                    m_focusedBodyIndex = -1; // return to overview
                }
                else
                {
                    startTourTransition();
                }
            }
        }
        return; // Don't do manual tracking while touring
    }

    // ── Continuous Camera Tracking ────────────────────────────────────────────
    if (m_focusedBodyIndex >= 0)
    {
        const auto& bodies = m_solarSystem.getBodies();
        if (m_focusedBodyIndex < static_cast<int>(bodies.size()))
        {
            m_camera.setTarget(bodies[m_focusedBodyIndex].position);
        }
    }
    else
    {
        // Overview (-1) focuses the Origin where the Sun is
        m_camera.setTarget(glm::vec3(0.0f));
    }
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void InputManager::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods)
{
    if (!s_instance) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            s_instance->m_isDragging = true;
            glfwGetCursorPos(win, &s_instance->m_lastMouseX, &s_instance->m_lastMouseY);
            s_instance->cancelTour(); // Addition 2: Manual drag cancels tour
        }
        else if (action == GLFW_RELEASE)
        {
            s_instance->m_isDragging = false;
        }
    }
}

void InputManager::cursorPosCallback(GLFWwindow* /*win*/, double xpos, double ypos)
{
    if (!s_instance) return;

    if (s_instance->m_isDragging)
    {
        double dx = xpos - s_instance->m_lastMouseX;
        double dy = ypos - s_instance->m_lastMouseY;
        s_instance->m_lastMouseX = xpos;
        s_instance->m_lastMouseY = ypos;

        // CG concept: Camera Orbit Control
        // Mouse X motion maps to Yaw (horizontal orbit). Mouse Y maps to Pitch (vertical orbit).
        const float sensitivity = 0.005f;
        s_instance->m_camera.adjustYaw(static_cast<float>(-dx) * sensitivity);
        s_instance->m_camera.adjustPitch(static_cast<float>(dy) * sensitivity);
    }
}

void InputManager::scrollCallback(GLFWwindow* /*win*/, double /*xoffset*/, double yoffset)
{
    if (!s_instance) return;
    
    s_instance->cancelTour(); // Addition 2: Manual zoom cancels tour

    // Zoom speed based on current distance (logarithmic feel)
    float currentDist = s_instance->m_camera.getDistance();
    float delta = static_cast<float>(-yoffset) * currentDist * 0.1f;
    s_instance->m_camera.adjustDistance(delta);
}

void InputManager::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if (!s_instance) return;
    if (action != GLFW_PRESS) return; // Only trigger on press

    // 1. Focus controls
    if (key == GLFW_KEY_ESCAPE)
    {
        s_instance->m_focusedBodyIndex = -1; // Overview (origin)
        s_instance->cancelTour();
    }
    else if (key == GLFW_KEY_0)
    {
        s_instance->m_focusedBodyIndex = 0;  // Sun
        s_instance->cancelTour();
    }
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8)
    {
        s_instance->m_focusedBodyIndex = key - GLFW_KEY_0; // Planets 1-8
        s_instance->cancelTour();
    }
    else if (key == GLFW_KEY_T) // Addition 2: Toggle Tour
    {
        if (s_instance->m_tourState == TourState::Off)
        {
            s_instance->m_tourTargetIndex = 0; // Start at Sun
            s_instance->startTourTransition();
        }
        else
        {
            s_instance->cancelTour();
        }
    }

    // 2. Simulation controls (Phase 10)
    if (key == GLFW_KEY_SPACE)
    {
        s_instance->m_paused = !s_instance->m_paused;
    }
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) // Plus key
    {
        s_instance->m_timeScale = std::min(s_instance->m_timeScale + 0.2f, 10.0f);
    }
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) // Minus key
    {
        s_instance->m_timeScale = std::max(s_instance->m_timeScale - 0.2f, 0.1f);
    }
    else if (key == GLFW_KEY_R)
    {
        s_instance->m_timeScale = 1.0f;
        s_instance->m_paused = false;
    }
}

} // namespace input
