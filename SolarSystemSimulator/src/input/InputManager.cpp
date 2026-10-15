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

void InputManager::toggleTour()
{
    if (m_tourState == TourState::Off)
        startTour();
    else
        cancelTour();
}

void InputManager::startTour()
{
    m_focusedBodyIndex = -1;
    m_focusedAsteroidIndex = -1;
    m_tourTargetIndex = 0;
    startTourTransition();
}

void InputManager::focusBody(int index)
{
    cancelTour();
    m_focusedAsteroidIndex = -1;
    m_focusedBodyIndex = index;
    if (index >= 0 && index < static_cast<int>(m_solarSystem.getBodies().size()))
    {
        const auto& body = m_solarSystem.getBodies()[index];
        float targetDistance = (index == 0) ? body.radius * 4.2f : body.radius * 2.7f;
        m_camera.setDistance(targetDistance);
        if (glm::length(body.position) > 0.1f)
        {
            glm::vec3 toSun = glm::normalize(-body.position);
            m_camera.setYaw(std::atan2(toSun.x, toSun.z) + 0.35f);
            m_camera.setPitch(0.22f);
        }
    }
}

void InputManager::focusAsteroid(int index)
{
    cancelTour();
    m_focusedBodyIndex = -1;
    m_focusedAsteroidIndex = index;
    if (index >= 0 && index < static_cast<int>(m_solarSystem.getAsteroids().size()))
    {
        const auto& ast = m_solarSystem.getAsteroids()[index];
        float targetDistance = std::max(ast.body.radius * 6.5f, 0.40f);
        m_camera.setDistance(targetDistance);
        if (glm::length(ast.body.position) > 0.1f)
        {
            glm::vec3 toSun = glm::normalize(-ast.body.position);
            m_camera.setYaw(std::atan2(toSun.x, toSun.z) + 0.35f);
            m_camera.setPitch(0.22f);
        }
    }
}

void InputManager::update(float deltaTime)
{
    // ── Addition 2: Cinematic Tour Mode ───────────────────────────────────────
    if (m_tourState != TourState::Off)
    {
        const int totalStops = m_solarSystem.getTourStopCount();
        if (m_tourTargetIndex >= totalStops)
            m_tourTargetIndex = 0; // fallback safety
            
        auto target = m_solarSystem.getTourStop(m_tourTargetIndex);
        
        // Framing stats: close enough to see all texture features & colors
        float targetDistance = target.radius * (m_tourTargetIndex == 0 ? 4.2f : 2.7f);
        if (target.radius < 0.12f)
        {
            targetDistance = std::max(target.radius * 6.5f, 0.40f); // Close-up view for small asteroids
        }

        // Align camera dynamically to view the planet from the sunlit side
        float targetYaw = 0.0f;
        if (glm::length(target.position) > 0.1f)
        {
            glm::vec3 toSun = glm::normalize(-target.position);
            targetYaw = std::atan2(toSun.x, toSun.z) + 0.35f; // 20-deg offset for 3/4 sunlit angle
        }
        float targetPitch = 0.22f; // Slight downward view showing rings, bands & poles
        
        if (m_tourState == TourState::Transitioning)
        {
            m_tourTimer += deltaTime;
            const float transitionDuration = 2.0f;
            
            float t = std::clamp(m_tourTimer / transitionDuration, 0.0f, 1.0f);
            
            // CG concept: Interpolation / Easing (Smoothstep)
            float easedT = t * t * (3.0f - 2.0f * t);
            
            m_camera.setTarget(glm::mix(m_tourStartTarget, target.position, easedT));
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
            const float dwellDuration = 4.8f; // Syncs with 3 Fun Facts x 1.6s each
            
            // Track the body dynamically as it orbits from the sunlit side
            m_camera.setTarget(target.position);
            m_camera.setDistance(targetDistance);
            m_camera.setYaw(targetYaw);
            m_camera.setPitch(targetPitch);
            
            if (m_tourTimer >= dwellDuration)
            {
                m_tourTargetIndex++;
                if (m_tourTargetIndex >= totalStops)
                {
                    cancelTour();
                    m_focusedBodyIndex = -1;
                    m_focusedAsteroidIndex = -1;
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
    else if (m_focusedAsteroidIndex >= 0)
    {
        const auto& asteroids = m_solarSystem.getAsteroids();
        if (m_focusedAsteroidIndex < static_cast<int>(asteroids.size()))
        {
            m_camera.setTarget(asteroids[m_focusedAsteroidIndex].body.position);
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
            s_instance->cancelTour(); // Manual drag cancels tour
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

        // Camera Orbit Control: dx -> Yaw, dy -> Pitch
        const float sensitivity = 0.005f;
        s_instance->m_camera.adjustYaw(static_cast<float>(-dx) * sensitivity);
        s_instance->m_camera.adjustPitch(static_cast<float>(dy) * sensitivity);
    }
}

void InputManager::scrollCallback(GLFWwindow* /*win*/, double /*xoffset*/, double yoffset)
{
    if (!s_instance) return;
    
    s_instance->cancelTour(); // Manual zoom cancels tour

    // Zoom speed based on current distance
    float currentDist = s_instance->m_camera.getDistance();
    float delta = static_cast<float>(-yoffset) * currentDist * 0.1f;
    s_instance->m_camera.adjustDistance(delta);
}

void InputManager::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods)
{
    if (!s_instance) return;
    if (action != GLFW_PRESS) return;

    // 1. Focus controls
    if (key == GLFW_KEY_ESCAPE)
    {
        s_instance->m_focusedBodyIndex = -1;
        s_instance->m_focusedAsteroidIndex = -1;
        s_instance->cancelTour();
    }
    else if (key == GLFW_KEY_0)
    {
        s_instance->m_focusedBodyIndex = 0;  // Sun
        s_instance->m_focusedAsteroidIndex = -1;
        s_instance->cancelTour();
    }
    else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8)
    {
        s_instance->m_focusedBodyIndex = key - GLFW_KEY_0; // Planets 1-8
        s_instance->m_focusedAsteroidIndex = -1;
        s_instance->cancelTour();
    }
    else if (key == GLFW_KEY_T) // Toggle Tour Mode
    {
        s_instance->toggleTour();
    }

    // 2. Simulation controls
    if (key == GLFW_KEY_SPACE)
    {
        s_instance->m_paused = !s_instance->m_paused;
    }
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
    {
        s_instance->m_timeScale = std::min(s_instance->m_timeScale + 0.2f, 10.0f);
    }
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
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
