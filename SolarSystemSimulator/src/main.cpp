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

        // ── Solar system scene data ──────────────────────────────────────────
        scene::SolarSystem solarSystem;
        const auto& bodies = solarSystem.getBodies();

        // ── Camera — positioned to see the full +X layout ────────────────────
        // The planets stretch from X=4 (Mercury) to X=32 (Neptune).
        // Target the midpoint (X=18); pull back ~28 units and look from above.
        //
        // To see just the inner planets:  setDistance(12), target=(7,0,0)
        // To see just the Sun close-up:   setDistance(6),  target=(0,0,0)
        // To see everything at max zoom:  setDistance(35), target=(16,0,0)
        camera::Camera camera(
            glm::vec3(18.0f, 0.0f, 0.0f),  // target: midpoint of the layout
            28.0f,                           // distance: enough to see Neptune
            glm::radians(10.0f),             // yaw:   slight angle (10°)
            glm::radians(28.0f)              // pitch: look from above (28°)
        );

        // ── FPS tracking ─────────────────────────────────────────────────────
        double lastTime   = glfwGetTime();
        int    frameCount = 0;

        // ══════════════════════════════════════════════════════════════════════
        // RENDER LOOP
        // ══════════════════════════════════════════════════════════════════════
        while (!window.shouldClose())
        {
            // ── Clear ─────────────────────────────────────────────────────────
            glClearColor(0.02f, 0.02f, 0.08f, 1.0f);   // deep space
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // ── View + Projection (constant for this frame) ───────────────────
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(window.aspectRatio());

            // ── Draw each celestial body ──────────────────────────────────────
            shader.use();
            shader.setMat4("uView",       view);
            shader.setMat4("uProjection", proj);

            for (const auto& body : bodies)
            {
                // CG concept – Model matrix (T * S):
                //   1. Scale the unit sphere to the body's visual radius.
                //   2. Translate it to its world-space position.
                //   Order: scale first (in local space), then translate.
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, body.position);
                model = glm::scale(model, glm::vec3(body.radius));

                shader.setMat4("uModel", model);
                shader.setVec3("uColor", body.color);  // per-body colour

                sphereMesh.draw();
            }

            // ── Swap + poll ───────────────────────────────────────────────────
            window.swapBuffers();
            window.pollEvents();

            // ── FPS display ───────────────────────────────────────────────────
            ++frameCount;
            double now  = glfwGetTime();
            double diff = now - lastTime;
            if (diff >= 1.0)
            {
                char buf[128];
                snprintf(buf, sizeof(buf),
                         "Solar System Simulator — Phase 4  |  %d bodies  |  %.0f FPS",
                         (int)bodies.size(), frameCount / diff);
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
