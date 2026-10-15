#pragma once
// =============================================================================
// UIOverlay.h  —  Dear ImGui integration for planet info display
// Extended (Feature 1): Performance Telemetry Overlay, GPU Timer Queries
// =============================================================================

#include "render/RenderStats.h"
#include <glad/glad.h>

struct GLFWwindow;
namespace scene { class SolarSystem; class AsteroidBelt; }
namespace input { class InputManager; }
namespace render { class Bloom; }  // Feature 2: forward declaration

namespace ui {

class UIOverlay
{
public:
    UIOverlay(GLFWwindow* window);
    ~UIOverlay();

    /// Called at the start of the render loop (after glfwPollEvents).
    void beginFrame();

    /// Wrap these two calls around the 3D scene render to measure GPU time.
    /// Uses double-buffering so the CPU never stalls waiting for GPU results.
    void beginGpuTimer();
    void endGpuTimer();

    /// Call once per frame after 3D rendering and before renderPanel().
    /// @param frameTimeMs  Raw delta time in milliseconds
    /// @param stats        Counters reset and filled each frame in main.cpp
    void updateFrameStats(float frameTimeMs, const render::RenderStats& stats);

    /// Renders all UI panels (main info panel + optional performance overlay).
    /// @param bloom  Feature 2: Bloom settings are read/written by the Observatory tab.
    void renderPanel(const scene::SolarSystem& solarSystem,
                     scene::AsteroidBelt& asteroidBelt,
                     input::InputManager& inputManager,
                     render::Bloom& bloom);

    /// Called at the end of the render loop (before swap buffers).
    void endFrame();

    /// Toggle the performance overlay window.  Bind to F3 key in main.cpp.
    void togglePerfOverlay()        { m_showPerfOverlay = !m_showPerfOverlay; }
    bool isPerfOverlayShown() const { return m_showPerfOverlay; }

    // Objects tab toggles (read by main.cpp render loop)
    bool showMoonOrbits() const  { return m_showMoonOrbits; }
    bool showCometTails() const  { return m_showCometTails; }

private:
    // ── Main panel state ─────────────────────────────────────────────────────
    int  m_compareBodyA    = 0;
    int  m_compareBodyB    = 3;
    bool m_showMoonOrbits  = true;
    bool m_showCometTails  = true;

    // ── Feature 1: Performance telemetry ring buffer ──────────────────────────
    static constexpr int FRAME_BUFFER_SIZE = 300;

    /// Circular buffer of frame times (ms).
    /// ImGui::PlotLines accepts a ring buffer directly via values_offset.
    float m_frameTimes[FRAME_BUFFER_SIZE] = {};
    int   m_frameHead  = 0;  ///< Next slot to write
    int   m_frameCount = 0;  ///< Valid entries so far (capped at FRAME_BUFFER_SIZE)

    float               m_lastFrameTimeMs = 0.0f;
    render::RenderStats m_lastStats       = {};

    bool  m_showPerfOverlay = false;

    // Cached statistics recomputed each frame by computeBufferStats()
    float m_avgFps       = 0.0f;
    float m_avgFrameMs   = 0.0f;
    float m_minFrameMs   = 0.0f;
    float m_maxFrameMs   = 0.0f;
    float m_p1LowFrameMs = 0.0f; ///< Average of worst 1% of frame times (high ms = bad FPS)

    void computeBufferStats();
    void renderPerfOverlay();
    void writePerfLog() const;

    // ── Feature 1: OpenGL GPU timer queries (GL_TIME_ELAPSED, core in 3.3) ────
    /// Two queries alternate each frame so we read last frame's result
    /// without stalling the pipeline.
    GLuint m_gpuQueries[2]     = {0, 0};
    int    m_queryWriteIndex   = 0;            ///< Query currently being recorded
    bool   m_queryReady[2]     = {false, false};
    float  m_gpuTimeMs         = 0.0f;         ///< Most recent completed GPU frame time
    bool   m_gpuQuerySupported = true;
};

} // namespace ui
