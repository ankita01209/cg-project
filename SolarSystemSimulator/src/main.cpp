// =============================================================================
// main.cpp — Solar System Simulator, Phase 4
//
// New in this phase:
//   • SolarSystem object owns all 9 body records (Sun + 8 planets)
//   • Render loop iterates over bodies, building a per-body model matrix
//     (translate to position, scale by visual radius)
//   • Camera repositioned to see the whole +X-axis layout at once
//   • Fragment shader now receives a uColor uniform per draw call
//
// CG concepts demonstrated:
//   • Scene graph (flat list of transforms)
//   • Model matrix decomposition: T * S (translate, then scale)
//   • Instancing pattern: one GPU mesh, many draw calls with different matrices
// =============================================================================

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "core/Window.h"
#include "core/Shader.h"
#include "camera/Camera.h"
#include "render/Sphere.h"
#include "scene/SolarSystem.h"
#include "input/InputManager.h"
#include "ui/UIOverlay.h"

// ── Shader asset paths ────────────────────────────────────────────────────────
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

        // ── Shader ──────────────────────────────────────────────────────────
        core::Shader shader(VERT_PATH, FRAG_PATH);

        // ── Shared sphere mesh ───────────────────────────────────────────────
        // CG concept – Geometry instancing pattern:
        //   All 9 bodies share ONE unit-sphere mesh on the GPU. Each draw call
        //   uses a different model matrix (position + scale) and a different
        //   uColor uniform to produce visually distinct bodies with zero
        //   extra geometry cost.
        render::Mesh sphereMesh = render::createSphere(1.0f, 48, 64);

        // ── Phase 12: Visual Polish Buffers ──────────────────────────────────
        core::Shader unlitShader(std::string(ASSETS_DIR) + "shaders/unlit.vert", std::string(ASSETS_DIR) + "shaders/unlit.frag");

        // ── Addition 4: Atmospheric Earth Shader ──────────────────────────────
        core::Shader atmoShader(std::string(ASSETS_DIR) + "shaders/atmosphere.vert", std::string(ASSETS_DIR) + "shaders/atmosphere.frag");

        // 1. Orbit Ring (XZ circle)
        GLuint ringVAO, ringVBO;
        std::vector<glm::vec3> ringPts;
        for(int i = 0; i < 100; ++i) {
            float a = static_cast<float>(i) / 100.0f * glm::two_pi<float>();
            ringPts.push_back({std::cos(a), 0.0f, std::sin(a)});
        }
        glGenVertexArrays(1, &ringVAO); glGenBuffers(1, &ringVBO);
        glBindVertexArray(ringVAO);
        glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
        glBufferData(GL_ARRAY_BUFFER, ringPts.size() * sizeof(glm::vec3), ringPts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        // 2. Starfield (Sphere of points)
        GLuint starVAO, starVBO;
        std::vector<glm::vec3> starPts;
        for(int i = 0; i < 800; ++i) {
            float u = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float v = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float theta = u * 2.0f * glm::pi<float>();
            float phi = std::acos(2.0f * v - 1.0f);
            float r = 400.0f;
            starPts.push_back({r * std::sin(phi) * std::cos(theta), r * std::cos(phi), r * std::sin(phi) * std::sin(theta)});
        }
        glGenVertexArrays(1, &starVAO); glGenBuffers(1, &starVBO);
        glBindVertexArray(starVAO);
        glBindBuffer(GL_ARRAY_BUFFER, starVBO);
        glBufferData(GL_ARRAY_BUFFER, starPts.size() * sizeof(glm::vec3), starPts.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        // 3. Sun Glow Billboard Quad (with UVs)
        GLuint glowVAO, glowVBO;
        float quadData[] = {
            // Pos (x,y,z),                 // UV (u,v) mapped to layout=2
            -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,    1.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,    1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f,    0.0f, 1.0f
        };
        glGenVertexArrays(1, &glowVAO); glGenBuffers(1, &glowVBO);
        glBindVertexArray(glowVAO);
        glBindBuffer(GL_ARRAY_BUFFER, glowVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);

        // ── Solar system scene data ──────────────────────────────────────────
        scene::SolarSystem solarSystem;
        const auto& bodies = solarSystem.getBodies();

        // Addition 1: Setup Saturn Ring Mesh
        // We initialize this once based on Saturn's specific inner/outer radius scales.
        render::Mesh saturnRingMesh = render::createRing(bodies[6].ringInnerRadius, bodies[6].ringOuterRadius, 64);

        const glm::vec3 CAMERA_TARGET        = glm::vec3(0.0f, 0.0f, 0.0f); // the Sun
        const float     CAMERA_DISTANCE      = 83.0f;  // optimal margin solved numerically
        const float     CAMERA_YAW_DEGREES   = 0.0f;
        const float     CAMERA_PITCH_DEGREES = 35.0f;
        const float     CAMERA_FOV_DEGREES   = 45.0f;
        const float     CAMERA_NEAR          = 0.1f;
        const float     CAMERA_FAR           = 1000.0f; // must exceed distance + orbit radius

        camera::Camera camera(
            CAMERA_TARGET,
            CAMERA_DISTANCE,
            glm::radians(CAMERA_YAW_DEGREES),
            glm::radians(CAMERA_PITCH_DEGREES),
            CAMERA_FOV_DEGREES
        );

        // ── Phase 9: Camera Controls ─────────────────────────────────────────
        input::InputManager inputManager(window, camera, solarSystem);

        // ── Phase 11: Planet Info UI ─────────────────────────────────────────
        ui::UIOverlay uiOverlay(window.handle());

        // ── FPS / Delta time tracking ────────────────────────────────────────
        double lastTime   = glfwGetTime();
        double lastFrame  = lastTime;
        int    frameCount = 0;

        // ══════════════════════════════════════════════════════════════════════
        // RENDER LOOP
        // ══════════════════════════════════════════════════════════════════════
        while (!window.shouldClose())
        {
            double now       = glfwGetTime();
            float  rawDelta  = static_cast<float>(now - lastFrame);
            lastFrame        = now;

            // ── Input & Tracking Update (Phase 9) ─────────────────────────────
            inputManager.update(rawDelta);

            // ── Update Simulation (Phase 10) ──────────────────────────────────
            if (!inputManager.isPaused())
            {
                solarSystem.update(rawDelta * inputManager.getTimeScale());
            }

            // ── Clear ─────────────────────────────────────────────────────────
            glClearColor(0.02f, 0.02f, 0.08f, 1.0f);   // deep space
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            uiOverlay.beginFrame();

            // ── View + Projection (constant for this frame) ───────────────────
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(window.aspectRatio());

            // ── Draw each celestial body ──────────────────────────────────────
            shader.use();
            shader.setMat4("uView",       view);
            shader.setMat4("uProjection", proj);
            
            // Phase 8: Set global lighting uniforms
            shader.setVec3("uLightPos", bodies.empty() ? glm::vec3(0.0f) : bodies[0].position);
            shader.setVec3("uViewPos", camera.getPosition());

            for (size_t i = 0; i < bodies.size(); ++i)
            {
                const auto& body = bodies[i];
                
                glm::mat4 model = body.getModelMatrix();
                shader.setMat4("uModel", model);
                shader.setVec3("uColor", body.color);

                shader.setInt("uIsEmissive", (i == 0) ? 1 : 0);
                sphereMesh.draw();
            }

            // ── Phase 7: Draw the Moon ────────────────────────────────────────
            glm::mat4 moonModel = solarSystem.getMoonModelMatrix();
            shader.setMat4("uModel", moonModel);
            shader.setVec3("uColor", solarSystem.getMoon().color);
            shader.setInt("uIsEmissive", 0); 
            sphereMesh.draw();

            // ── TRANSPARENT PASS ──────────────────────────────────────────────
            // Drawn after opaque geometry. Depth test ON, depth write OFF.
            glDepthMask(GL_FALSE);
            
            // ── Addition 4: Atmospheric Earth ─────────────────────────────────
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure additive glow
            glEnable(GL_CULL_FACE); // Render back-facing culled, we only want the front hemisphere's rim
            
            atmoShader.use();
            atmoShader.setMat4("uView", view);
            atmoShader.setMat4("uProjection", proj);
            atmoShader.setVec3("uViewPos", camera.getPosition());
            atmoShader.setVec3("uColor", glm::vec3(0.2f, 0.5f, 1.0f)); // soft blue tint
            
            {
                const auto& earth = bodies[3]; // Earth
                glm::mat4 earthPos = glm::translate(glm::mat4(1.0f), earth.position);
                glm::mat4 atmoScale = glm::scale(glm::mat4(1.0f), glm::vec3(earth.radius * 1.15f));
                
                // CG concept: Hierarchical transform omitting spin
                // We only multiply translation and scale, ignoring self-rotation.
                atmoShader.setMat4("uModel", earthPos * atmoScale);
                sphereMesh.draw();
            }

            // ── Restore generic transparent state ───────────────────────────────
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // ── Addition 1: Saturn's Rings ────────────────────────────────────
            unlitShader.use();
            unlitShader.setMat4("uView", view);
            unlitShader.setMat4("uProjection", proj);
            unlitShader.setInt("uIsBillboard", 0);
            unlitShader.setVec4("uColor", glm::vec4(0.85f, 0.8f, 0.7f, 0.75f)); // pale tan
            
            glDisable(GL_CULL_FACE); // Render ring from both sides
            const auto& saturn = bodies[6];
            // Hierarchical local-to-world: Parent orbit * fixed tilt * relative scale
            glm::mat4 saturnTrans = glm::translate(glm::mat4(1.0f), saturn.position);
            glm::mat4 ringTilt    = glm::rotate(glm::mat4(1.0f), saturn.ringTiltAngle, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::mat4 ringScale   = glm::scale(glm::mat4(1.0f), glm::vec3(saturn.radius));
            
            // Note: Does NOT inherit saturn.selfRotationAngle !
            unlitShader.setMat4("uModel", saturnTrans * ringTilt * ringScale);
            saturnRingMesh.draw();
            glEnable(GL_CULL_FACE);

            // ── Phase 12: Starfield ───────────────────────────────────────────
            unlitShader.setMat4("uModel", glm::mat4(1.0f));
            unlitShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));
            glBindVertexArray(starVAO);
            glDrawArrays(GL_POINTS, 0, 800);

            // ── Phase 12: Draw Orbit Rings ────────────────────────────────────
            unlitShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 0.15f));
            glBindVertexArray(ringVAO);
            
            for (size_t i = 1; i < bodies.size(); ++i)
            {
                // Planet orbits are centered on Sun (origin)
                const auto& body = bodies[i];
                glm::mat4 ringModel = glm::scale(glm::mat4(1.0f), glm::vec3(body.orbitDistance));
                unlitShader.setMat4("uModel", ringModel);
                glDrawArrays(GL_LINE_LOOP, 0, 100);
            }
            
            // Moon orbit ring is centered on Earth
            {
                const auto& earth = bodies[3]; // Earth is body[3]
                glm::mat4 earthTrans = glm::translate(glm::mat4(1.0f), earth.position);
                glm::mat4 moonRingModel = earthTrans * glm::scale(glm::mat4(1.0f), glm::vec3(solarSystem.getMoon().orbitDistance * earth.radius));
                unlitShader.setMat4("uModel", moonRingModel);
                glDrawArrays(GL_LINE_LOOP, 0, 100);
            }
            
            // ── Phase 12: Draw Sun Glow ───────────────────────────────────────
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
            unlitShader.setInt("uIsBillboard", 1);
            unlitShader.setVec4("uColor", glm::vec4(bodies[0].color, 0.7f));
            
            // Model = scale(sun.radius * 3.5), translate to 0 (since sun is at 0)
            glm::mat4 glowModel = glm::scale(glm::mat4(1.0f), glm::vec3(bodies[0].radius * 3.5f));
            unlitShader.setMat4("uModel", glowModel);
            glBindVertexArray(glowVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            // Restore state
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);

            // ── Phase 11: Draw UI Overlay ─────────────────────────────────────
            uiOverlay.renderPanel(solarSystem, inputManager);
            uiOverlay.endFrame();

            // ── Swap + poll ───────────────────────────────────────────────────
            window.swapBuffers();
            window.pollEvents();

            // ── FPS display ───────────────────────────────────────────────────
            ++frameCount;
            double diff = now - lastTime;
            if (diff >= 1.0)
            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Solar System Simulator  |  %d bodies  |  %.0f FPS",
                         (int)bodies.size() + 1, frameCount / diff);
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
