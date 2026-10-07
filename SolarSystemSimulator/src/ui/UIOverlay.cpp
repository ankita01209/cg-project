// =============================================================================
// UIOverlay.cpp
// =============================================================================

#include "ui/UIOverlay.h"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "scene/SolarSystem.h"
#include "input/InputManager.h"
#include <string>
#include <algorithm>
#include <vector>

namespace ui {

UIOverlay::UIOverlay(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark(); // Phase 12 prep: styling

    // Phase 12 styling: Give ImGui a clean dark tone matching deep space
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05f, 0.05f, 0.07f, 0.85f);
    style.Colors[ImGuiCol_TitleBg]  = ImVec4(0.1f, 0.15f, 0.3f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.25f, 0.5f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
}

UIOverlay::~UIOverlay()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UIOverlay::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIOverlay::renderPanel(const scene::SolarSystem& solarSystem, const input::InputManager& inputManager)
{
    // Minimalist transparent panel
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 280), ImGuiCond_FirstUseEver);

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

                // 2. Focused Planet Info
                const auto& bodies = solarSystem.getBodies();
                
                // Addition 2: Cinematic Tour Info
                if (inputManager.getTourState() != input::InputManager::TourState::Off)
                {
                    int tourIdx = inputManager.getTourTargetIndex();
                    if (tourIdx >= 0 && tourIdx < static_cast<int>(bodies.size()))
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "[ Touring: %s ]", bodies[tourIdx].name.c_str());
                    }
                }

                int focusIdx = inputManager.getFocusedBodyIndex();
                
                if (focusIdx == -1)
                {
                    ImGui::Text("Target: Free / Overview (Press 0-8 to focus)");
                }
                else
                {
                    if (focusIdx < static_cast<int>(bodies.size()))
                    {
                        const auto& body = bodies[focusIdx];
                        ImGui::Text("Target: %s", body.name.c_str());
                        ImGui::Text("Orbit Distance: %.1f wu", body.orbitDistance);
                        ImGui::Text("Radius: %.2f wu", body.radius);
                        ImGui::Text("Orbital velocity: %.2f rad/s", body.orbitalSpeed);
                        ImGui::Text("Rotation velocity: %.2f rad/s", body.rotationSpeed);
                    }
                }

                ImGui::Separator();
                ImGui::TextWrapped("Controls:\nLeft-click + Drag: Orbit Camera\nScroll: Zoom\n1-8: Focus Planet\n0: Focus Sun\nEsc: Overview\nSpace: Pause\n+/-: Adjust Speed\nR: Reset\nT: Toggle Tour Mode");
                
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
                    ImGui::Text("Radius:\n  %.2f", bA.radius);
                    ImGui::Text("Orbit Dist:\n  %.1f", bA.orbitDistance);
                    ImGui::Text("Orbit Spd:\n  %.2f", bA.orbitalSpeed);
                    ImGui::Text("Rot Spd:\n  %.2f", bA.rotationSpeed);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextColored(ImVec4(bB.color.r, bB.color.g, bB.color.b, 1.0f), "[ %s ]", bB.name.c_str());
                    ImGui::Text("Radius:\n  %.2f", bB.radius);
                    ImGui::Text("Orbit Dist:\n  %.1f", bB.orbitDistance);
                    ImGui::Text("Orbit Spd:\n  %.2f", bB.orbitalSpeed);
                    ImGui::Text("Rot Spd:\n  %.2f", bB.rotationSpeed);
                    
                    ImGui::EndTable();
                }

                ImGui::Separator();

                // Derived comparisons
                if (bB.radius > 0.0f) {
                    float sizeRatio = bA.radius / bB.radius;
                    ImGui::TextWrapped("> %s is %.1fx the radius of %s.", bA.name.c_str(), sizeRatio, bB.name.c_str());
                }
                if (bB.orbitalSpeed > 0.0f) {
                    float spdRatio = bA.orbitalSpeed / bB.orbitalSpeed;
                    if (spdRatio > 0.0f)
                        ImGui::TextWrapped("> %s orbits %.1fx faster than %s.", bA.name.c_str(), spdRatio, bB.name.c_str());
                }
                ImGui::TextDisabled("\n(Note: comparisons use simulation constraints,\nnot real-world astronomical figures).");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void UIOverlay::endFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace ui
