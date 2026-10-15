// =============================================================================
// UIOverlay.cpp
// =============================================================================

#include "ui/UIOverlay.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "render/Bloom.h"       // Feature 2: Bloom settings written by Observatory tab
#include "scene/SolarSystem.h"
#include "scene/AsteroidBelt.h"
#include "input/InputManager.h"
#include <string>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cfloat>
#include <fstream>
#include <iomanip>

namespace ui {

namespace {
    // Session 2 Addition 3: Hardcoded Fun Facts per body
    const char* getFunFact(int bodyIndex, int factIndex) {
        static const char* facts[9][3] = {
            // Sun (0)
            {"The Sun is the only emissive light source in the simulation.",
             "It uses a special unlit shader to bypass Blinn-Phong shading.",
             "A volumetric glow billboard surrounds it."},
            // Mercury (1)
            {"Fastest orbital speed in the system.",
             "Smallest planet, closest to the Sun.",
             "No atmosphere to retain heat."},
            // Venus (2)
            {"Rotates backwards compared to most planets.",
             "Visually distinct due to its dense cover.",
             "Second planet from the Sun."},
            // Earth (3)
            {"Features a custom procedural Fresnel atmosphere.",
             "Home to a hierarchically orbiting Moon.",
             "The only planet with liquid water."},
            // Mars (4)
            {"The Red Planet, coloured by iron oxide.",
             "Roughly half the size of Earth in this simulator.",
             "Has two tiny moons (not rendered here)."},
            // Jupiter (5)
            {"The largest planet, dwarfing all others.",
             "Could fit over 1,000 Earths inside.",
             "Famous for its Great Red Spot."},
            // Saturn (6)
            {"Features procedural annulus rings built from a triangle strip.",
             "Rings are tilted at a fixed 26.7 degrees independent of spin.",
             "The second largest planet."},
            // Uranus (7)
            {"An ice giant with a distinctive pale cyan hue.",
             "Rotates entirely on its side.",
             "Seventh planet from the Sun."},
            // Neptune (8)
            {"The outermost planet, orbiting the slowest.",
             "Its wide orbit requires a far camera clipping plane over 80.0 units.",
             "Known for supersonic wind speeds."}
        };
        if (bodyIndex < 0 || bodyIndex >= 9) return "";
        return facts[bodyIndex][factIndex % 3];
    }
} // anonymous namespace

// =============================================================================
// Construction / Destruction
// =============================================================================

UIOverlay::UIOverlay(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Deep-space theme
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.05f, 0.05f, 0.07f, 0.85f);
    style.Colors[ImGuiCol_TitleBg]       = ImVec4(0.1f,  0.15f, 0.3f,  1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.25f, 0.5f,  1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // Feature 1: Double-buffered GPU timer queries.
    // GL_TIME_ELAPSED is core in OpenGL 3.3 (via GL_ARB_timer_query).
    glGenQueries(2, m_gpuQueries);
    if (m_gpuQueries[0] == 0) m_gpuQuerySupported = false;
}

UIOverlay::~UIOverlay()
{
    // Feature 1: Release GPU query objects before context is destroyed.
    if (m_gpuQueries[0] != 0) glDeleteQueries(2, m_gpuQueries);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// =============================================================================
// Frame boundaries
// =============================================================================

void UIOverlay::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIOverlay::endFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// =============================================================================
// Feature 1: GPU Timer Query  (wrap around the 3D scene render in main.cpp)
// =============================================================================

void UIOverlay::beginGpuTimer()
{
    if (!m_gpuQuerySupported) return;
    glBeginQuery(GL_TIME_ELAPSED, m_gpuQueries[m_queryWriteIndex]);
}

void UIOverlay::endGpuTimer()
{
    if (!m_gpuQuerySupported) return;

    glEndQuery(GL_TIME_ELAPSED);
    m_queryReady[m_queryWriteIndex] = true;

    // Read the OTHER slot (previous frame) — non-blocking: check AVAILABLE first.
    const int readIndex = 1 - m_queryWriteIndex;
    if (m_queryReady[readIndex])
    {
        GLint available = 0;
        glGetQueryObjectiv(m_gpuQueries[readIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available)
        {
            GLuint elapsed = 0;
            glGetQueryObjectuiv(m_gpuQueries[readIndex], GL_QUERY_RESULT, &elapsed);
            m_gpuTimeMs = static_cast<float>(elapsed) * 1e-6f; // nanoseconds → ms
        }
    }
    m_queryWriteIndex = 1 - m_queryWriteIndex; // swap for next frame
}

// =============================================================================
// Feature 1: Frame Statistics
// =============================================================================

void UIOverlay::updateFrameStats(float frameTimeMs, const render::RenderStats& stats)
{
    // Write into ring buffer
    m_frameTimes[m_frameHead] = frameTimeMs;
    m_frameHead = (m_frameHead + 1) % FRAME_BUFFER_SIZE;
    if (m_frameCount < FRAME_BUFFER_SIZE) ++m_frameCount;

    m_lastFrameTimeMs = frameTimeMs;
    m_lastStats = stats;

    computeBufferStats();
}

void UIOverlay::computeBufferStats()
{
    if (m_frameCount == 0) return;

    // Build a linear snapshot for sorting (stack allocation of 300 floats = 1.2 KB)
    float temp[FRAME_BUFFER_SIZE];
    float sum    = 0.0f;
    float minVal = FLT_MAX;
    float maxVal = 0.0f;

    const int startIdx = (m_frameHead - m_frameCount + FRAME_BUFFER_SIZE) % FRAME_BUFFER_SIZE;
    for (int i = 0; i < m_frameCount; ++i)
    {
        const float v = m_frameTimes[(startIdx + i) % FRAME_BUFFER_SIZE];
        temp[i] = v;
        sum    += v;
        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
    }

    m_avgFrameMs = sum / static_cast<float>(m_frameCount);
    m_avgFps     = (m_avgFrameMs > 0.0f) ? (1000.0f / m_avgFrameMs) : 0.0f;
    m_minFrameMs = (minVal < FLT_MAX) ? minVal : 0.0f;
    m_maxFrameMs = maxVal;

    // 1% low: sort descending (highest ms = worst performance), average top 1%.
    std::sort(temp, temp + m_frameCount, std::greater<float>());
    const int p1Count = std::max(1, m_frameCount / 100);
    float p1Sum = 0.0f;
    for (int i = 0; i < p1Count; ++i) p1Sum += temp[i];
    m_p1LowFrameMs = p1Sum / static_cast<float>(p1Count);
}

// =============================================================================
// Feature 1: Performance Overlay Window
// =============================================================================

void UIOverlay::renderPerfOverlay()
{
    if (!m_showPerfOverlay) return;

    ImGui::SetNextWindowPos(ImVec2(20.0f, 330.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 415.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.88f);

    // p_open = &m_showPerfOverlay lets the X button close the window
    if (!ImGui::Begin("Performance Telemetry  [F3]", &m_showPerfOverlay,
                      ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return;
    }

    // ── Frame-Time Plot ───────────────────────────────────────────────────────
    // ImGui::PlotLines natively supports ring buffers: pass values_offset = m_frameHead.
    {
        char label[64];
        snprintf(label, sizeof(label), "Now %.2f ms", m_lastFrameTimeMs);
        ImGui::Text("Frame Time (ms)   %d / %d frames", m_frameCount, FRAME_BUFFER_SIZE);
        const float yMax = std::max(m_maxFrameMs * 1.25f, 5.0f);
        ImGui::PlotLines("##ft", m_frameTimes, FRAME_BUFFER_SIZE, m_frameHead,
                         label, 0.0f, yMax, ImVec2(-1.0f, 72.0f));
    }

    // ── FPS Plot ─────────────────────────────────────────────────────────────
    {
        // Convert the ring buffer to FPS on the stack
        float fpsData[FRAME_BUFFER_SIZE];
        for (int i = 0; i < FRAME_BUFFER_SIZE; ++i)
            fpsData[i] = (m_frameTimes[i] > 0.0f) ? (1000.0f / m_frameTimes[i]) : 0.0f;

        const float nowFps = (m_lastFrameTimeMs > 0.0f) ? (1000.0f / m_lastFrameTimeMs) : 0.0f;
        const float fpsMax = std::max(m_avgFps * 1.5f, 30.0f);
        char label[64];
        snprintf(label, sizeof(label), "Now %.0f FPS", nowFps);
        ImGui::Text("FPS");
        ImGui::PlotLines("##fps", fpsData, FRAME_BUFFER_SIZE, m_frameHead,
                         label, 0.0f, fpsMax, ImVec2(-1.0f, 72.0f));
    }

    ImGui::Separator();

    // ── Aggregate Statistics ──────────────────────────────────────────────────
    ImGui::Text("Average :  %6.2f ms   (%5.0f FPS)", m_avgFrameMs, m_avgFps);
    ImGui::Text("Min     :  %6.2f ms   (%5.0f FPS)", m_minFrameMs,
                m_minFrameMs > 0.0f ? 1000.0f / m_minFrameMs : 0.0f);
    ImGui::Text("Max     :  %6.2f ms   (%5.0f FPS)", m_maxFrameMs,
                m_maxFrameMs > 0.0f ? 1000.0f / m_maxFrameMs : 0.0f);
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f),
                       "1%% Low :  %6.2f ms   (%5.0f FPS)",
                       m_p1LowFrameMs,
                       m_p1LowFrameMs > 0.0f ? 1000.0f / m_p1LowFrameMs : 0.0f);

    ImGui::Separator();

    // ── Draw Call Statistics ──────────────────────────────────────────────────
    ImGui::Text("Draw calls        : %d", m_lastStats.drawCalls);
    ImGui::Text("Instanced draws   : %d", m_lastStats.instancedDrawCalls);
    ImGui::Text("Total instances   : %d", m_lastStats.totalInstances);
    ImGui::Text("Triangles / frame : %d", m_lastStats.triangleCount);

    ImGui::Separator();

    // ── GPU Time ─────────────────────────────────────────────────────────────
    if (m_gpuQuerySupported)
        ImGui::Text("GPU time          : %.3f ms", m_gpuTimeMs);
    else
        ImGui::TextDisabled("GPU time          : (GL_TIME_ELAPSED unavailable)");

    ImGui::Separator();

    // ── Export Button ─────────────────────────────────────────────────────────
    if (ImGui::Button("Write  performance_log.txt"))
    {
        writePerfLog();
        ImGui::OpenPopup("perf_saved");
    }
    if (ImGui::BeginPopup("perf_saved"))
    {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Saved to performance_log.txt");
        ImGui::EndPopup();
    }

    ImGui::End();
}

void UIOverlay::writePerfLog() const
{
    std::ofstream f("performance_log.txt");
    if (!f.is_open()) return;

    f << std::fixed;
    f << "Solar System Simulator -- Performance Summary\n";
    f << "Samples  : " << m_frameCount << " frames\n\n";

    f << "[Frame Time Statistics]\n";
    f << std::setprecision(3);
    f << "  Average   : " << m_avgFrameMs   << " ms   ("
      << std::setprecision(0) << m_avgFps   << " FPS)\n";
    f << std::setprecision(3);
    f << "  Minimum   : " << m_minFrameMs   << " ms\n";
    f << "  Maximum   : " << m_maxFrameMs   << " ms\n";
    f << "  1% Low    : " << m_p1LowFrameMs << " ms   ("
      << std::setprecision(0)
      << (m_p1LowFrameMs > 0.0f ? 1000.0f / m_p1LowFrameMs : 0.0f) << " FPS)\n\n";

    f << "[Draw Call Statistics -- last frame]\n";
    f << "  Draw calls      : " << m_lastStats.drawCalls          << "\n";
    f << "  Instanced draws : " << m_lastStats.instancedDrawCalls  << "\n";
    f << "  Total instances : " << m_lastStats.totalInstances       << "\n";
    f << "  Triangles       : " << m_lastStats.triangleCount        << "\n";

    if (m_gpuQuerySupported)
    {
        f << std::setprecision(3);
        f << "\n[GPU]\n";
        f << "  GPU time (last frame) : " << m_gpuTimeMs << " ms\n";
    }
}

// =============================================================================
// Main panel (all existing tabs preserved, F3 shortcut + checkbox added)
// =============================================================================

void UIOverlay::renderPanel(const scene::SolarSystem& solarSystem,
                            scene::AsteroidBelt& asteroidBelt,
                            input::InputManager& inputManager,
                            render::Bloom& bloom)
{
    // Minimalist transparent panel
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 290), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Solar System Simulator", nullptr, ImGuiWindowFlags_NoCollapse))
    {
        if (ImGui::BeginTabBar("MainTabs"))
        {
            if (ImGui::BeginTabItem("Focus Info"))
            {
                // 1. Simulation controls state
                if (inputManager.isPaused())
                    ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "[ PAUSED ]");
                else
                    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "[ RUNNING ]");

                ImGui::Text("Time Scale: %.1fx", inputManager.getTimeScale());
                ImGui::Separator();

                // 2. Tour indicator
                const auto& bodies = solarSystem.getBodies();
                if (inputManager.getTourState() != input::InputManager::TourState::Off)
                {
                    int tourIdx = inputManager.getTourTargetIndex();
                    if (tourIdx >= 0 && tourIdx < solarSystem.getTourStopCount())
                    {
                        scene::TourStopInfo stop = solarSystem.getTourStop(tourIdx);
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                                           "[ Touring: %s ]", stop.name.c_str());
                    }
                    if (ImGui::Button("Stop Tour [T]"))
                        inputManager.toggleTour();
                }
                else
                {
                    if (ImGui::Button("Start Cinematic Tour [T]"))
                        inputManager.startTour();
                }

                // 3. Focused body info
                int focusIdx = inputManager.getFocusedBodyIndex();
                int astFocusIdx = inputManager.getFocusedAsteroidIndex();
                if (astFocusIdx >= 0 && astFocusIdx < static_cast<int>(solarSystem.getAsteroids().size()))
                {
                    const auto& ast = solarSystem.getAsteroids()[astFocusIdx];
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Target (Asteroid): %s", ast.name.c_str());
                    ImGui::Text("Category: %s", ast.designation.c_str());
                    ImGui::Text("Orbit Distance: %.2f wu", ast.semiMajor);
                    ImGui::Text("Radius: %.3f wu", ast.body.radius);
                    ImGui::Text("Inclination: %.1f deg", glm::degrees(ast.inclination));
                    ImGui::Text("Orbital velocity: %.2f rad/s", ast.orbitalSpeed);
                    if (ImGui::Button("Reset to Overview [Esc]"))
                    {
                        inputManager.focusBody(-1);
                    }
                }
                else if (focusIdx == -1)
                {
                    ImGui::Text("Target: Free / Overview (Press 0-8 to focus)");
                }
                else if (focusIdx < static_cast<int>(bodies.size()))
                {
                    const auto& body = bodies[focusIdx];
                    ImGui::Text("Target: %s",               body.name.c_str());
                    ImGui::Text("Orbit Distance: %.1f wu",  body.orbitDistance);
                    ImGui::Text("Radius: %.2f wu",          body.radius);
                    ImGui::Text("Orbital velocity: %.2f rad/s",  body.orbitalSpeed);
                    ImGui::Text("Rotation velocity: %.2f rad/s", body.rotationSpeed);
                }

                ImGui::Separator();
                // Feature 1: F3 added to the cheat sheet
                ImGui::TextWrapped(
                    "Controls:\n"
                    "Left-click + Drag : Orbit Camera\n"
                    "Scroll            : Zoom\n"
                    "1-8               : Focus Planet\n"
                    "0                 : Focus Sun\n"
                    "Esc               : Overview\n"
                    "Space             : Pause\n"
                    "+/-               : Adjust Speed\n"
                    "R                 : Reset Speed\n"
                    "T                 : Toggle Tour Mode\n"
                    "F3                : Perf Overlay\n"
                    "F9                : Diag CSV (120 fr)");
                ImGui::Separator();
                // Feature 1: inline checkbox mirrors F3 toggle
                ImGui::Checkbox("Performance Overlay (F3)", &m_showPerfOverlay);

                ImGui::EndTabItem();
            }

            // ── Addition 3: Planet Comparison Mode ────────────────────────────
            if (ImGui::BeginTabItem("Compare"))
            {
                const auto& bodies = solarSystem.getBodies();
                std::vector<const char*> names;
                for (const auto& b : bodies) names.push_back(b.name.c_str());

                ImGui::Combo("Body A", &m_compareBodyA, names.data(), static_cast<int>(names.size()));
                ImGui::Combo("Body B", &m_compareBodyB, names.data(), static_cast<int>(names.size()));

                int safeA = std::clamp(m_compareBodyA, 0, static_cast<int>(bodies.size()) - 1);
                int safeB = std::clamp(m_compareBodyB, 0, static_cast<int>(bodies.size()) - 1);
                const auto& bA = bodies[safeA];
                const auto& bB = bodies[safeB];

                ImGui::Separator();
                if (ImGui::BeginTable("compare_table", 2, ImGuiTableFlags_BordersInnerV))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(bA.color.r, bA.color.g, bA.color.b, 1.0f), "[ %s ]", bA.name.c_str());
                    ImGui::Text("Radius:\n  %.2f",    bA.radius);
                    ImGui::Text("Orbit Dist:\n  %.1f", bA.orbitDistance);
                    ImGui::Text("Orbit Spd:\n  %.2f",  bA.orbitalSpeed);
                    ImGui::Text("Rot Spd:\n  %.2f",    bA.rotationSpeed);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(bB.color.r, bB.color.g, bB.color.b, 1.0f), "[ %s ]", bB.name.c_str());
                    ImGui::Text("Radius:\n  %.2f",    bB.radius);
                    ImGui::Text("Orbit Dist:\n  %.1f", bB.orbitDistance);
                    ImGui::Text("Orbit Spd:\n  %.2f",  bB.orbitalSpeed);
                    ImGui::Text("Rot Spd:\n  %.2f",    bB.rotationSpeed);

                    ImGui::EndTable();
                }
                ImGui::Separator();
                if (bB.radius > 0.0f)
                    ImGui::TextWrapped("> %s is %.1fx the radius of %s.",
                                       bA.name.c_str(), bA.radius / bB.radius, bB.name.c_str());
                if (bB.orbitalSpeed > 0.0f) {
                    float r = bA.orbitalSpeed / bB.orbitalSpeed;
                    if (r > 0.0f)
                        ImGui::TextWrapped("> %s orbits %.1fx faster than %s.",
                                           bA.name.c_str(), r, bB.name.c_str());
                }
                ImGui::TextDisabled("\n(Note: values use simulation constraints,\nnot real-world astronomical figures).");
                ImGui::EndTabItem();
            }

            // ── Objects Browser: Moons + Comets ────────────────────────────────
            if (ImGui::BeginTabItem("Objects"))
            {
                const auto& moons  = solarSystem.getMoons();
                const auto& comets = solarSystem.getComets();
                const auto& bodies = solarSystem.getBodies();

                ImGui::Checkbox("Show Moon Orbits", &m_showMoonOrbits);
                ImGui::SameLine();
                ImGui::Checkbox("Show Comet Tails", &m_showCometTails);
                ImGui::Separator();

                if (ImGui::CollapsingHeader("Moons", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < static_cast<int>(moons.size()); ++i)
                    {
                        const auto& rec = moons[i];
                        const auto& b   = rec.body;
                        ImGui::PushID(i);
                        ImGui::TextColored(
                            ImVec4(b.color.r, b.color.g, b.color.b, 1.0f),
                            "%s", b.name.c_str());
                        ImGui::SameLine();
                        const char* parentName = (rec.parentIndex < static_cast<int>(bodies.size()))
                            ? bodies[rec.parentIndex].name.c_str() : "?";
                        ImGui::TextDisabled("-> %s  r=%.3f  orbit=%.2f",
                                           parentName, b.radius, b.orbitDistance);
                        ImGui::PopID();
                    }
                }
                if (ImGui::CollapsingHeader("Comets", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (int i = 0; i < static_cast<int>(comets.size()); ++i)
                    {
                        const auto& c = comets[i];
                        ImGui::PushID(1000 + i);
                        ImGui::TextColored(
                            ImVec4(c.body.color.r, c.body.color.g, c.body.color.b, 1.0f),
                            "%s", c.name.c_str());
                        ImGui::TextDisabled(" a=%.1f wu  e=%.2f", c.semiMajor, c.eccentricity);
                        ImGui::TextWrapped(" %s", c.description.c_str());
                        ImGui::Spacing();
                        ImGui::PopID();
                    }
                }

                // ── Asteroids & Minor Planets ────────────────────────────────
                if (ImGui::CollapsingHeader("Asteroids & Minor Planets", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    const auto& asteroids = solarSystem.getAsteroids();
                    for (int i = 0; i < static_cast<int>(asteroids.size()); ++i)
                    {
                        const auto& ast = asteroids[i];
                        const auto& b   = ast.body;
                        ImGui::PushID(2000 + i);
                        ImGui::TextColored(
                            ImVec4(b.color.r, b.color.g, b.color.b, 1.0f),
                            "%s", ast.name.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled("[%s]", ast.designation.c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Focus"))
                        {
                            inputManager.focusAsteroid(i);
                        }
                        ImGui::TextDisabled("  Orbit: %.2f wu | Inclination: %.1f deg | r: %.3f",
                                            ast.semiMajor, glm::degrees(ast.inclination), b.radius);
                        if (ImGui::TreeNode("Fun Facts"))
                        {
                            for (int f = 0; f < 3; ++f)
                                ImGui::BulletText("%s", ast.facts[f].c_str());
                            ImGui::TreePop();
                        }
                        ImGui::Spacing();
                        ImGui::PopID();
                    }
                }
                ImGui::EndTabItem();
            }

            // ── Addition 4: Solar Observatory ─────────────────────────────────
            if (ImGui::BeginTabItem("Observatory"))
            {
                if (ImGui::CollapsingHeader("Global Settings", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    bool texturesEnabled = solarSystem.isTexturesEnabled();
                    if (ImGui::Checkbox("Surface Textures", &texturesEnabled))
                        solarSystem.setTexturesEnabled(texturesEnabled);
                }
                if (ImGui::CollapsingHeader("Asteroid Belt", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    bool beltEnabled = asteroidBelt.isEnabled();
                    if (ImGui::Checkbox("Enable Asteroid Belt", &beltEnabled))
                        asteroidBelt.setEnabled(beltEnabled);

                    int count = asteroidBelt.getCount();
                    ImGui::Text("Region: Mars (11.0 wu) to Jupiter (15.0 wu)");
                    if (beltEnabled)
                        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                                           "Status: ON | Instanced Field: %d rocks", count);
                    else
                        ImGui::TextDisabled("Status: OFF");

                    ImGui::Text("Instanced Count:");
                    if (ImGui::RadioButton("150", count == 150)) asteroidBelt.setCount(150); ImGui::SameLine();
                    if (ImGui::RadioButton("250", count == 250)) asteroidBelt.setCount(250); ImGui::SameLine();
                    if (ImGui::RadioButton("300", count == 300)) asteroidBelt.setCount(300);

                    ImGui::Spacing();
                    ImGui::Text("Major Recognized Asteroids:");
                    ImGui::BulletText("1 Ceres (Dwarf planet, contains 1/3 belt mass)");
                    ImGui::BulletText("4 Vesta (Brightest protoplanet, Rheasilvia basin)");
                    ImGui::BulletText("2 Pallas (Steep 34.8 deg inclination, golf-ball surface)");
                    ImGui::BulletText("10 Hygiea (Dark carbonaceous dwarf planet candidate)");
                    ImGui::BulletText("433 Eros (Peanut-shaped Near-Earth asteroid)");
                    ImGui::BulletText("16 Psyche (Exposed nickel-iron planetary core)");
                    if (ImGui::Button("Tour Asteroids & Solar System [T]"))
                        inputManager.startTour();
                }

                // ── Feature 2: Bloom / HDR controls ──────────────────────────
                if (ImGui::CollapsingHeader("Bloom / HDR", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Checkbox("Enable Bloom",       &bloom.enabled);
                    ImGui::BeginDisabled(!bloom.enabled);
                    ImGui::SliderFloat("Intensity##bloom",  &bloom.intensity,  0.0f, 2.0f,  "%.2f");
                    ImGui::SliderFloat("Threshold##bloom",  &bloom.threshold,  0.3f, 2.0f,  "%.2f");
                    ImGui::SliderInt("Blur Passes##bloom",  &bloom.blurPasses, 1,    10);
                    ImGui::EndDisabled();
                    ImGui::SliderFloat("Exposure##bloom",   &bloom.exposure,   0.1f, 3.0f,  "%.2f");
                    ImGui::TextDisabled("(Exposure applies even without bloom)");
                }
                if (ImGui::CollapsingHeader("Space Missions", ImGuiTreeNodeFlags_DefaultOpen))
                    ImGui::TextDisabled("(Missions built in Phase 2)");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // Feature 1: Performance telemetry overlay (separate ImGui window)
    renderPerfOverlay();

    // ── Session 2 Addition 2: "Now Visiting: X" Title Card ────────────────────
    if (inputManager.getTourState() == input::InputManager::TourState::Transitioning)
    {
        float tourTimer = inputManager.getTourTimer();
        float alpha = 0.0f;
        if      (tourTimer < 0.5f) alpha = tourTimer / 0.5f;
        else if (tourTimer < 1.5f) alpha = 1.0f;
        else                       alpha = std::max(0.0f, 1.0f - ((tourTimer - 1.5f) / 0.5f));

        if (alpha > 0.001f)
        {
            int tourIdx = inputManager.getTourTargetIndex();
            scene::TourStopInfo stop = solarSystem.getTourStop(tourIdx);
            std::string titleStr = "Now Visiting: " + stop.name;
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            if (ImGui::Begin("TitleCard", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoBackground))
            {
                ImGui::SetWindowFontScale(3.0f);
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, alpha), "%s", titleStr.c_str());
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::End();
        }
    }
    // ── Session 2 Addition 3: Rotating Fun Facts During Dwell ─────────────────
    else if (inputManager.getTourState() == input::InputManager::TourState::Dwelling)
    {
        float tourTimer = inputManager.getTourTimer();
        const float factDuration = 1.6f;
        int   factIndex  = static_cast<int>(std::floor(tourTimer / factDuration));
        float localTimer = std::fmod(tourTimer, factDuration);

        float alpha = 0.0f;
        if      (localTimer < 0.3f) alpha = localTimer / 0.3f;
        else if (localTimer < 1.3f) alpha = 1.0f;
        else                        alpha = std::max(0.0f, 1.0f - ((localTimer - 1.3f) / 0.3f));

        if (alpha > 0.001f)
        {
            int tourIdx = inputManager.getTourTargetIndex();
            scene::TourStopInfo stop = solarSystem.getTourStop(tourIdx);
            const char* factStr = stop.facts[factIndex % 3];

            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(ImVec2(center.x, center.y + 150.0f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg,
                                  ImVec4(0.05f, 0.1f, 0.2f, alpha * 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));

            if (ImGui::Begin("FunFacts", nullptr,
                             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, alpha), "DID YOU KNOW?");
                ImGui::Separator();
                ImGui::SetWindowFontScale(1.3f);
                ImGui::TextUnformatted(factStr);
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::End();
            ImGui::PopStyleColor(2);
        }
    }
}

} // namespace ui
