#pragma once
// =============================================================================
// UIOverlay.h — Dear ImGui integration for planet info display
// =============================================================================

struct GLFWwindow;
namespace scene { class SolarSystem; }
namespace input { class InputManager; }

namespace ui {

class UIOverlay
{
public:
    UIOverlay(GLFWwindow* window);
    ~UIOverlay();

    // Must be called at the start of the render loop (after glfwPollEvents ideally)
    void beginFrame();
    
    // Renders the actual UI panels
    void renderPanel(const scene::SolarSystem& solarSystem, const input::InputManager& inputManager);
    
    // Must be called at the end of the render loop (before swap buffers)
    void endFrame();

private:
    int m_compareBodyA = 0; // Default: Sun
    int m_compareBodyB = 3; // Default: Earth
};

} // namespace ui
