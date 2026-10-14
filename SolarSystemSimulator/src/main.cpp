// =============================================================================
// main.cpp — Solar System Simulator
//
// Feature 1: render::RenderStats, GPU timer queries, F3 overlay, F9 CSV
// Feature 2: render::Bloom — HDR FBO capture, Gaussian bloom, Reinhard tone-map
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <string>

#include "core/Window.h"
#include "core/Shader.h"
#include "camera/Camera.h"
#include "render/Sphere.h"
#include "render/RenderStats.h"
#include "render/Bloom.h"
#include "scene/SolarSystem.h"
#include "scene/AsteroidBelt.h"
#include "input/InputManager.h"
#include "ui/UIOverlay.h"

#ifndef ASSETS_DIR
#   define ASSETS_DIR "assets/"
#endif
static const std::string VERT_PATH = std::string(ASSETS_DIR) + "shaders/basic.vert";
static const std::string FRAG_PATH = std::string(ASSETS_DIR) + "shaders/basic.frag";

// =============================================================================
int main()
{
    try
    {
        // ── Window + OpenGL context ──────────────────────────────────────────
        core::Window window(1280, 720, "Solar System Simulator — Phase 4");

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        // ── Shaders ──────────────────────────────────────────────────────────
        core::Shader shader(VERT_PATH, FRAG_PATH);
        core::Shader unlitShader(std::string(ASSETS_DIR) + "shaders/unlit.vert",
                                 std::string(ASSETS_DIR) + "shaders/unlit.frag");
        core::Shader atmoShader(std::string(ASSETS_DIR) + "shaders/atmosphere.vert",
                                std::string(ASSETS_DIR) + "shaders/atmosphere.frag");

        // ── Shared sphere mesh ───────────────────────────────────────────────
        render::Mesh sphereMesh = render::createSphere(1.0f, 48, 64);

        // ── Phase 12: Orbit Ring (XZ circle) ─────────────────────────────────
        GLuint ringVAO, ringVBO;
        {
            std::vector<glm::vec3> ringPts;
            for (int i = 0; i < 100; ++i) {
                float a = static_cast<float>(i) / 100.0f * glm::two_pi<float>();
                ringPts.push_back({std::cos(a), 0.0f, std::sin(a)});
            }
            glGenVertexArrays(1, &ringVAO); glGenBuffers(1, &ringVBO);
            glBindVertexArray(ringVAO);
            glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
            glBufferData(GL_ARRAY_BUFFER,
                         ringPts.size() * sizeof(glm::vec3), ringPts.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        }

        // ── Phase 12: Starfield ───────────────────────────────────────────────
        GLuint starVAO, starVBO;
        {
            std::vector<glm::vec3> starPts;
            for (int i = 0; i < 800; ++i) {
                float u     = static_cast<float>(rand()) / RAND_MAX;
                float v     = static_cast<float>(rand()) / RAND_MAX;
                float theta = u * 2.0f * glm::pi<float>();
                float phi   = std::acos(2.0f * v - 1.0f);
                float r     = 400.0f;
                starPts.push_back({r*std::sin(phi)*std::cos(theta),
                                   r*std::cos(phi),
                                   r*std::sin(phi)*std::sin(theta)});
            }
            glGenVertexArrays(1, &starVAO); glGenBuffers(1, &starVBO);
            glBindVertexArray(starVAO);
            glBindBuffer(GL_ARRAY_BUFFER, starVBO);
            glBufferData(GL_ARRAY_BUFFER,
                         starPts.size() * sizeof(glm::vec3), starPts.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        }

        // ── Phase 12: Sun Glow Billboard Quad ────────────────────────────────
        GLuint glowVAO, glowVBO;
        {
            float qd[] = {
                -1.f,-1.f,0.f, 0.f,0.f,
                 1.f,-1.f,0.f, 1.f,0.f,
                 1.f, 1.f,0.f, 1.f,1.f,
                -1.f,-1.f,0.f, 0.f,0.f,
                 1.f, 1.f,0.f, 1.f,1.f,
                -1.f, 1.f,0.f, 0.f,1.f
            };
            glGenVertexArrays(1, &glowVAO); glGenBuffers(1, &glowVBO);
            glBindVertexArray(glowVAO);
            glBindBuffer(GL_ARRAY_BUFFER, glowVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(qd), qd, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
                                  (void*)(3*sizeof(float)));
            glBindVertexArray(0);
        }

        // ── Scene data ───────────────────────────────────────────────────────
        scene::SolarSystem  solarSystem;
        scene::AsteroidBelt asteroidBelt;
        const auto& bodies = solarSystem.getBodies();

        render::Mesh saturnRingMesh = render::createRing(
            bodies[6].ringInnerRadius, bodies[6].ringOuterRadius, 64);

        // ── Camera ───────────────────────────────────────────────────────────
        camera::Camera camera(glm::vec3(0.0f), 83.0f,
                              glm::radians(0.0f), glm::radians(35.0f), 45.0f);

        // ── Systems ──────────────────────────────────────────────────────────
        input::InputManager inputManager(window, camera, solarSystem);
        ui::UIOverlay       uiOverlay(window.handle());

        // ── Feature 2: Bloom post-processing ─────────────────────────────────
        render::Bloom bloom(std::string(ASSETS_DIR));
        {
            int fbW, fbH;
            glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
            bloom.resize(fbW, fbH);
        }

        // ── Feature 1: Per-frame counters ────────────────────────────────────
        render::RenderStats stats;

        double lastTime   = glfwGetTime();
        double lastFrame  = lastTime;
        int    frameCount = 0;

        // ── F9 diagnostic state ───────────────────────────────────────────────
        static bool          s_f9WasPressed  = false;
        static bool          s_diagActive    = false;
        static int           s_diagFrames    = 0;
        static std::ofstream s_transformLog;
        static int           lastTourTarget  = -1;

        // ══════════════════════════════════════════════════════════════════════
        // RENDER LOOP
        // ══════════════════════════════════════════════════════════════════════
        while (!window.shouldClose())
        {
            double now      = glfwGetTime();
            float  rawDelta = static_cast<float>(now - lastFrame);
            lastFrame       = now;

            // ── Input & Simulation ────────────────────────────────────────────
            inputManager.update(rawDelta);
            if (!inputManager.isPaused())
            {
                float sc = rawDelta * inputManager.getTimeScale();
                solarSystem.update(sc);
                asteroidBelt.update(sc);
            }

            // ── DIAGNOSTICS: F9 Hook ──────────────────────────────────────────
            {
                bool triggered = false;
                int  tourIndex = (inputManager.getTourState() != input::InputManager::TourState::Off)
                               ? inputManager.getTourTargetIndex() : -1;
                if (inputManager.getTourState() == input::InputManager::TourState::Dwelling
                    && tourIndex != lastTourTarget)
                {
                    if (tourIndex == 1 || tourIndex == 3 || tourIndex == 5) triggered = true;
                    lastTourTarget = tourIndex;
                }

                bool f9Now = (glfwGetKey(window.handle(), GLFW_KEY_F9) == GLFW_PRESS) || triggered;
                if (f9Now && !s_f9WasPressed)
                {
                    s_diagActive = true;
                    s_diagFrames = 120;
                    printf("\n[DEBUG] F9 Pressed: Starting 120-frame diagnostic log...\n");
                    s_transformLog.open("transform_log.csv");
                    s_transformLog << "Frame,Time,TourState,TourTarget,"
                                      "PlanPosX,PlanPosY,PlanPosZ,"
                                      "CamPosX,CamPosY,CamPosZ,"
                                      "Near,Far,FrameTimeMs\n";
                }
                s_f9WasPressed = f9Now;

                if (s_diagActive && s_diagFrames > 0)
                {
                    if (s_diagFrames == 120)
                        printf("[DEBUG] ============ ONE-FRAME DRAW DUMP ============\n");

                    int ti = (inputManager.getTourState() != input::InputManager::TourState::Off)
                           ? inputManager.getTourTargetIndex() : -1;
                    if (ti >= 0 && ti < static_cast<int>(bodies.size()))
                    {
                        const auto& tg     = bodies[ti];
                        glm::vec3   camPos = camera.getPosition();
                        s_transformLog << std::fixed << std::setprecision(4)
                            << (120 - s_diagFrames + 1) << "," << glfwGetTime() << ","
                            << static_cast<int>(inputManager.getTourState()) << "," << ti << ","
                            << tg.position.x  << "," << tg.position.y  << "," << tg.position.z << ","
                            << camPos.x << "," << camPos.y << "," << camPos.z << ",,"
                            << (rawDelta * 1000.0f) << "\n";
                    }
                }
                else if (s_diagActive && s_diagFrames == 0)
                {
                    s_diagActive = false;
                    if (s_transformLog.is_open()) s_transformLog.close();
                    printf("[DEBUG] F9 Diagnostic complete.\n");
                }
            }

            // ── Feature 2: handle window resize ──────────────────────────────
            {
                int fbW, fbH;
                glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
                bloom.resize(fbW, fbH);  // no-op if size unchanged
            }

            // ── Begin ImGui frame (must come BEFORE GPU timer) ────────────────
            uiOverlay.beginFrame();

            // ── Feature 1: reset counters; start GPU timer ────────────────────
            stats.reset();
            uiOverlay.beginGpuTimer();

            // ── Feature 2: redirect scene render to HDR FBO ───────────────────
            if (bloom.isValid())
                bloom.beginSceneCapture(); // binds HDR FBO, clears it
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                int fbW, fbH;
                glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
                glViewport(0, 0, fbW, fbH);
                glClearColor(0.02f, 0.02f, 0.08f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }

            // Restore 3-D pipeline state (bloom.process() disables them)
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            // ── View / Projection ────────────────────────────────────────────
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(window.aspectRatio());

            // ── Draw each celestial body (opaque pass) ────────────────────────
            shader.use();
            shader.setMat4("uView",       view);
            shader.setMat4("uProjection", proj);
            shader.setVec3("uLightPos",   bodies.empty() ? glm::vec3(0.0f) : bodies[0].position);
            shader.setVec3("uViewPos",    camera.getPosition());
            shader.setInt("uGlobalTextureEnabled", solarSystem.isTexturesEnabled() ? 1 : 0);
            shader.setInt("uTexture", 0);

            int highlightIndex =
                (inputManager.getTourState() != input::InputManager::TourState::Off)
                ? inputManager.getTourTargetIndex()
                : inputManager.getFocusedBodyIndex();

            for (size_t i = 0; i < bodies.size(); ++i)
            {
                const auto& body  = bodies[i];
                shader.setMat4("uModel",        body.getModelMatrix());
                shader.setVec3("uColor",        body.color);
                shader.setInt("uIsEmissive",    (i == 0) ? 1 : 0);
                shader.setInt("uIsHighlighted", 0);
                shader.setFloat("uAlpha",       1.0f);

                if (body.texture && body.texture->hasTexture()) {
                    body.texture->bind(0);
                    shader.setInt("uHasTexture", 1);
                } else {
                    shader.setInt("uHasTexture", 0);
                }

                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
            }

            if (s_diagActive && s_diagFrames > 0) s_diagFrames--;

            // ── Moon ─────────────────────────────────────────────────────────
            {
                shader.setMat4("uModel",        solarSystem.getMoonModelMatrix());
                shader.setVec3("uColor",        solarSystem.getMoon().color);
                shader.setInt("uIsEmissive",    0);
                shader.setInt("uIsHighlighted", 0);
                shader.setFloat("uAlpha",       1.0f);
                if (solarSystem.getMoon().texture && solarSystem.getMoon().texture->hasTexture()) {
                    solarSystem.getMoon().texture->bind(0);
                    shader.setInt("uHasTexture", 1);
                } else {
                    shader.setInt("uHasTexture", 0);
                }
                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
            }

            // ── Asteroid Belt (instanced) ─────────────────────────────────────
            shader.setInt("uHasTexture", 0);
            shader.setInt("uIsEmissive", 0);
            shader.setInt("uIsHighlighted", 0);
            shader.setFloat("uAlpha", 1.0f);
            asteroidBelt.draw(shader, &stats);

            // ── TRANSPARENT PASS ──────────────────────────────────────────────
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);

            // Rim-highlight shell
            if (highlightIndex > 0 && highlightIndex < static_cast<int>(bodies.size()))
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                shader.use();
                const auto& tgt       = bodies[highlightIndex];
                glm::mat4   shellModel = glm::scale(tgt.getModelMatrix(), glm::vec3(1.03f));
                shader.setMat4("uModel",          shellModel);
                shader.setVec3("uColor",          glm::vec3(0.0f, 0.7f, 1.0f));
                shader.setInt("uHasTexture",      0);
                shader.setInt("uIsHighlighted",   1);
                shader.setVec3("uHighlightColor", glm::vec3(0.0f, 0.7f, 1.0f));
                shader.setInt("uIsEmissive",      0);
                shader.setFloat("uAlpha",         0.4f);
                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
                shader.setInt("uIsHighlighted", 0);
                shader.setFloat("uAlpha", 1.0f);
            }

            // Atmospheric Fresnel glow (Earth)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glEnable(GL_CULL_FACE);
            atmoShader.use();
            atmoShader.setMat4("uView",       view);
            atmoShader.setMat4("uProjection", proj);
            atmoShader.setVec3("uViewPos",    camera.getPosition());
            atmoShader.setVec3("uColor",      glm::vec3(0.2f, 0.5f, 1.0f));
            {
                const auto& earth    = bodies[3];
                glm::mat4   earthPos = glm::translate(glm::mat4(1.0f), earth.position);
                glm::mat4   atmoSc   = glm::scale(glm::mat4(1.0f), glm::vec3(earth.radius * 1.15f));
                atmoShader.setMat4("uModel", earthPos * atmoSc);
                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
            }
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Saturn rings
            unlitShader.use();
            unlitShader.setMat4("uView",       view);
            unlitShader.setMat4("uProjection", proj);
            unlitShader.setInt("uIsBillboard",  0);
            unlitShader.setVec4("uColor",       glm::vec4(0.85f, 0.8f, 0.7f, 0.75f));
            glDisable(GL_CULL_FACE);
            {
                const auto& saturn   = bodies[6];
                glm::mat4   satTrans = glm::translate(glm::mat4(1.0f), saturn.position);
                glm::mat4   ringTilt = glm::rotate(glm::mat4(1.0f), saturn.ringTiltAngle,
                                                   glm::vec3(0, 0, 1));
                glm::mat4   ringSc   = glm::scale(glm::mat4(1.0f), glm::vec3(saturn.radius));
                unlitShader.setMat4("uModel", satTrans * ringTilt * ringSc);
                saturnRingMesh.draw();
                stats.recordDraw(saturnRingMesh.indexCount());
            }
            glEnable(GL_CULL_FACE);

            // Starfield (800 points)
            unlitShader.setMat4("uModel", glm::mat4(1.0f));
            unlitShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));
            glBindVertexArray(starVAO);
            glDrawArrays(GL_POINTS, 0, 800);
            ++stats.drawCalls;

            // Orbit rings (planets + Moon)
            unlitShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            glBindVertexArray(ringVAO);
            for (size_t i = 1; i < bodies.size(); ++i)
            {
                glm::mat4 rm = glm::scale(glm::mat4(1.0f), glm::vec3(bodies[i].orbitDistance));
                unlitShader.setMat4("uModel", rm);
                glDrawArrays(GL_LINE_LOOP, 0, 100);
                ++stats.drawCalls;
            }
            {
                const auto& earth     = bodies[3];
                glm::mat4   earthTrans = glm::translate(glm::mat4(1.0f), earth.position);
                glm::mat4   moonRingM  = earthTrans * glm::scale(glm::mat4(1.0f),
                    glm::vec3(solarSystem.getMoon().orbitDistance * earth.radius));
                unlitShader.setMat4("uModel", moonRingM);
                glDrawArrays(GL_LINE_LOOP, 0, 100);
                ++stats.drawCalls;
            }

            // Sun corona glow (additive billboard)
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            unlitShader.setInt("uIsBillboard", 1);
            unlitShader.setVec4("uColor", glm::vec4(bodies[0].color, 0.7f));
            {
                glm::mat4 gm = glm::scale(glm::mat4(1.0f),
                                          glm::vec3(bodies[0].radius * 3.5f));
                unlitShader.setMat4("uModel", gm);
                glBindVertexArray(glowVAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                ++stats.drawCalls;
                stats.triangleCount += 2;
            }

            // Restore state
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);

            // ── Feature 2: run bloom post-processing ─────────────────────────
            // endSceneCapture unbinds the HDR FBO; process() composites to default FB.
            if (bloom.isValid())
            {
                bloom.endSceneCapture();
                bloom.process();
            }

            // ── Feature 1: end GPU timer, update perf stats ───────────────────
            uiOverlay.endGpuTimer();
            uiOverlay.updateFrameStats(rawDelta * 1000.0f, stats);

            // ── Feature 1: F3 toggle (edge-detected) ─────────────────────────
            {
                static bool f3Was = false;
                bool f3Now = (glfwGetKey(window.handle(), GLFW_KEY_F3) == GLFW_PRESS);
                if (f3Now && !f3Was) uiOverlay.togglePerfOverlay();
                f3Was = f3Now;
            }

            // ── UI (drawn to default FB, on top of bloom composite) ───────────
            uiOverlay.renderPanel(solarSystem, asteroidBelt, inputManager, bloom);
            uiOverlay.endFrame();

            window.swapBuffers();
            window.pollEvents();

            // ── Window-title FPS ─────────────────────────────────────────────
            ++frameCount;
            if ((now - lastTime) >= 1.0)
            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Solar System Simulator  |  %d bodies  |  %.0f FPS  |  Bloom %s",
                         static_cast<int>(bodies.size()) + 1,
                         static_cast<double>(frameCount) / (now - lastTime),
                         bloom.enabled ? "ON" : "OFF");
                window.setTitle(buf);
                frameCount = 0;
                lastTime   = now;
            }
        }
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "\n[FATAL] %s\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
