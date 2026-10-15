// =============================================================================
// main.cpp — Solar System Simulator
//
// Feature 1: RenderStats, GPU timer, F3 overlay, F9 CSV
// Feature 2: HDR FBO + Bloom post-processing
// Feature 3: 14 moons, 2 comets with animated tails
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

        // ── Shared geometry ──────────────────────────────────────────────────
        render::Mesh sphereMesh = render::createSphere(1.0f, 48, 64);

        // ── Orbit Ring (XZ circle, 100 pts) ──────────────────────────────────
        GLuint ringVAO, ringVBO;
        {
            std::vector<glm::vec3> pts;
            for (int i = 0; i < 100; ++i) {
                float a = static_cast<float>(i) / 100.0f * glm::two_pi<float>();
                pts.push_back({std::cos(a), 0.0f, std::sin(a)});
            }
            glGenVertexArrays(1, &ringVAO); glGenBuffers(1, &ringVBO);
            glBindVertexArray(ringVAO);
            glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
            glBufferData(GL_ARRAY_BUFFER, pts.size()*sizeof(glm::vec3), pts.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        }

        // ── Starfield ─────────────────────────────────────────────────────────
        GLuint starVAO, starVBO;
        {
            std::vector<glm::vec3> stars;
            for (int i = 0; i < 800; ++i) {
                float u=static_cast<float>(rand())/RAND_MAX, v=static_cast<float>(rand())/RAND_MAX;
                float theta=u*2.f*glm::pi<float>(), phi=std::acos(2.f*v-1.f), r=400.f;
                stars.push_back({r*std::sin(phi)*std::cos(theta), r*std::cos(phi),
                                 r*std::sin(phi)*std::sin(theta)});
            }
            glGenVertexArrays(1,&starVAO); glGenBuffers(1,&starVBO);
            glBindVertexArray(starVAO);
            glBindBuffer(GL_ARRAY_BUFFER,starVBO);
            glBufferData(GL_ARRAY_BUFFER,stars.size()*sizeof(glm::vec3),stars.data(),GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(void*)0);
        }

        // ── Sun Glow Billboard ────────────────────────────────────────────────
        GLuint glowVAO, glowVBO;
        {
            float qd[]={-1,-1,0,0,0, 1,-1,0,1,0, 1,1,0,1,1, -1,-1,0,0,0, 1,1,0,1,1, -1,1,0,0,1};
            glGenVertexArrays(1,&glowVAO); glGenBuffers(1,&glowVBO);
            glBindVertexArray(glowVAO); glBindBuffer(GL_ARRAY_BUFFER,glowVBO);
            glBufferData(GL_ARRAY_BUFFER,sizeof(qd),qd,GL_STATIC_DRAW);
            glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
            glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
            glBindVertexArray(0);
        }

        // ── Comet tail billboard (thin elongated quad, 6 vertices) ───────────
        // Tail is rendered as a stretched quad along the tail direction.
        // We build it in local space [-0.5..0.5 along Y, -0.1..0.1 along X]
        // and transform it with a model matrix in the render loop.
        GLuint tailVAO, tailVBO;
        {
            // Narrow quad: 2 triangles, positions only (tail direction = local +Y)
            float tv[] = {
                -0.15f, 0.0f, 0.0f,
                 0.15f, 0.0f, 0.0f,
                 0.05f, 1.0f, 0.0f,
                -0.15f, 0.0f, 0.0f,
                 0.05f, 1.0f, 0.0f,
                -0.05f, 1.0f, 0.0f
            };
            glGenVertexArrays(1,&tailVAO); glGenBuffers(1,&tailVBO);
            glBindVertexArray(tailVAO); glBindBuffer(GL_ARRAY_BUFFER,tailVBO);
            glBufferData(GL_ARRAY_BUFFER,sizeof(tv),tv,GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
            glBindVertexArray(0);
        }

        // ── Scene data ───────────────────────────────────────────────────────
        scene::SolarSystem  solarSystem;
        scene::AsteroidBelt asteroidBelt;
        const auto& bodies = solarSystem.getBodies();

        render::Mesh saturnRingMesh = render::createRing(
            bodies[6].ringInnerRadius, bodies[6].ringOuterRadius, 64);

        // ── Camera ───────────────────────────────────────────────────────────
        camera::Camera camera(glm::vec3(0.0f), 70.0f,
                              glm::radians(0.0f), glm::radians(35.0f), 45.0f);

        // ── Systems ──────────────────────────────────────────────────────────
        input::InputManager inputManager(window, camera, solarSystem);
        ui::UIOverlay       uiOverlay(window.handle());

        // ── Bloom ────────────────────────────────────────────────────────────
        render::Bloom bloom(std::string(ASSETS_DIR));
        {
            int fbW, fbH;
            glfwGetFramebufferSize(window.handle(), &fbW, &fbH);
            bloom.resize(fbW, fbH);
        }

        render::RenderStats stats;
        double lastTime   = glfwGetTime();
        double lastFrame  = lastTime;
        int    frameCount = 0;

        // F9 diagnostic state
        static bool          s_f9WasPressed = false;
        static bool          s_diagActive   = false;
        static int           s_diagFrames   = 0;
        static std::ofstream s_transformLog;
        static int           lastTourTarget = -1;

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

            // ── F9 Diagnostic ─────────────────────────────────────────────────
            {
                bool triggered = false;
                int  tourIndex = (inputManager.getTourState() != input::InputManager::TourState::Off)
                               ? inputManager.getTourTargetIndex() : -1;
                if (inputManager.getTourState() == input::InputManager::TourState::Dwelling
                    && tourIndex != lastTourTarget)
                {
                    if (tourIndex==1||tourIndex==3||tourIndex==5) triggered=true;
                    lastTourTarget = tourIndex;
                }
                bool f9Now = (glfwGetKey(window.handle(), GLFW_KEY_F9)==GLFW_PRESS)||triggered;
                if (f9Now && !s_f9WasPressed)
                {
                    s_diagActive=true; s_diagFrames=120;
                    printf("\n[DEBUG] F9 Pressed: Starting 120-frame diagnostic log...\n");
                    s_transformLog.open("transform_log.csv");
                    s_transformLog<<"Frame,Time,TourState,TourTarget,PlanPosX,PlanPosY,PlanPosZ,"
                                    "CamPosX,CamPosY,CamPosZ,Near,Far,FrameTimeMs\n";
                }
                s_f9WasPressed = f9Now;
                if (s_diagActive && s_diagFrames>0)
                {
                    if (s_diagFrames==120) printf("[DEBUG] ============ ONE-FRAME DRAW DUMP ============\n");
                    int ti=(inputManager.getTourState()!=input::InputManager::TourState::Off)
                          ? inputManager.getTourTargetIndex():-1;
                    if (ti>=0 && ti<solarSystem.getTourStopCount())
                    {
                        auto stop=solarSystem.getTourStop(ti); glm::vec3 cp=camera.getPosition();
                        s_transformLog<<std::fixed<<std::setprecision(4)
                            <<(120-s_diagFrames+1)<<","<<glfwGetTime()<<","
                            <<static_cast<int>(inputManager.getTourState())<<","<<ti<<","
                            <<stop.position.x<<","<<stop.position.y<<","<<stop.position.z<<","
                            <<cp.x<<","<<cp.y<<","<<cp.z<<",,"<<(rawDelta*1000.f)<<"\n";
                    }
                }
                else if (s_diagActive && s_diagFrames==0)
                { s_diagActive=false; if(s_transformLog.is_open())s_transformLog.close();
                  printf("[DEBUG] F9 Diagnostic complete.\n"); }
            }

            // ── Bloom resize ──────────────────────────────────────────────────
            { int fbW,fbH; glfwGetFramebufferSize(window.handle(),&fbW,&fbH); bloom.resize(fbW,fbH); }

            uiOverlay.beginFrame();
            stats.reset();
            uiOverlay.beginGpuTimer();

            if (bloom.isValid()) bloom.beginSceneCapture();
            else {
                glBindFramebuffer(GL_FRAMEBUFFER,0);
                int fbW,fbH; glfwGetFramebufferSize(window.handle(),&fbW,&fbH);
                glViewport(0,0,fbW,fbH);
                glClearColor(0.02f,0.02f,0.08f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
            }
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE); glCullFace(GL_BACK);

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(window.aspectRatio());
            glm::vec3 sunPos = bodies[0].position;

            // ── Planets (opaque) ──────────────────────────────────────────────
            shader.use();
            shader.setMat4("uView",       view);
            shader.setMat4("uProjection", proj);
            shader.setVec3("uLightPos",   sunPos);
            shader.setVec3("uViewPos",    camera.getPosition());
            shader.setInt("uGlobalTextureEnabled", solarSystem.isTexturesEnabled()?1:0);
            shader.setInt("uTexture", 0);
            shader.setFloat("uSpecularStrength", 0.25f);
            shader.setFloat("uShininess", 32.0f);

            int highlightIndex = (inputManager.getTourState()!=input::InputManager::TourState::Off)
                               ? inputManager.getTourTargetIndex()
                               : inputManager.getFocusedBodyIndex();

            for (size_t i=0; i<bodies.size(); ++i)
            {
                const auto& body = bodies[i];
                shader.setMat4("uModel",        body.getModelMatrix());
                shader.setVec3("uColor",        body.color);
                shader.setInt("uIsEmissive",    (i==0)?1:0);
                shader.setInt("uIsHighlighted", 0);
                shader.setFloat("uAlpha",       1.0f);
                if (body.texture && body.texture->hasTexture()) {
                    body.texture->bind(0); shader.setInt("uHasTexture",1);
                } else shader.setInt("uHasTexture",0);
                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
            }
            if (s_diagActive && s_diagFrames>0) s_diagFrames--;

            // ── Earth's Moon (legacy path — index 0 in moons[]) ──────────────
            {
                shader.setMat4("uModel",        solarSystem.getMoonModelMatrix());
                shader.setVec3("uColor",        solarSystem.getMoon().color);
                shader.setInt("uIsEmissive",    0);
                shader.setInt("uIsHighlighted", 0);
                shader.setFloat("uAlpha",       1.0f);
                if (solarSystem.getMoon().texture && solarSystem.getMoon().texture->hasTexture()){
                    solarSystem.getMoon().texture->bind(0); shader.setInt("uHasTexture",1);
                } else shader.setInt("uHasTexture",0);
                sphereMesh.draw();
                stats.recordDraw(sphereMesh.indexCount());
            }

            // ── All other moons (indices 1+) ──────────────────────────────────
            {
                const auto& moons = solarSystem.getMoons();
                shader.setInt("uIsEmissive",    0);
                shader.setInt("uIsHighlighted", 0);
                for (int mi=1; mi<static_cast<int>(moons.size()); ++mi)
                {
                    const auto& rec = moons[mi];
                    const auto& b   = rec.body;
                    if (rec.parentIndex<0||rec.parentIndex>=static_cast<int>(bodies.size())) continue;

                    const glm::vec3& parentPos    = bodies[rec.parentIndex].position;
                    float            parentRadius  = bodies[rec.parentIndex].radius;
                    glm::mat4 moonModel = rec.getModelMatrix(parentPos, parentRadius);

                    shader.setMat4("uModel",    moonModel);
                    shader.setVec3("uColor",    b.color);
                    shader.setFloat("uAlpha",   1.0f);
                    if (b.texture && b.texture->hasTexture()){
                        b.texture->bind(0); shader.setInt("uHasTexture",1);
                    } else shader.setInt("uHasTexture",0);
                    sphereMesh.draw();
                    stats.recordDraw(sphereMesh.indexCount());
                }
            }

            // ── Comet nuclei ──────────────────────────────────────────────────
            {
                const auto& comets = solarSystem.getComets();
                shader.setInt("uIsEmissive",    0);
                shader.setInt("uIsHighlighted", 0);
                shader.setInt("uHasTexture",    0);
                for (const auto& c : comets)
                {
                    const auto& b = c.body;
                    glm::mat4 cm = glm::translate(glm::mat4(1.0f), b.position);
                    cm = glm::rotate(cm, b.selfRotationAngle, glm::vec3(0,1,0));
                    cm = glm::scale(cm, glm::vec3(b.radius));
                    shader.setMat4("uModel",   cm);
                    shader.setVec3("uColor",   b.color);
                    shader.setFloat("uAlpha",  1.0f);
                    sphereMesh.draw();
                    stats.recordDraw(sphereMesh.indexCount());
                }
            }

            // ── Named Asteroids (Ceres, Vesta, Pallas, Hygiea, Eros, Psyche) ──
            {
                const auto& asteroids = solarSystem.getAsteroids();
                shader.setInt("uIsEmissive",    0);
                shader.setInt("uIsHighlighted", 0);
                for (size_t ai = 0; ai < asteroids.size(); ++ai)
                {
                    const auto& ast = asteroids[ai];
                    const auto& b   = ast.body;
                    glm::mat4 am = glm::translate(glm::mat4(1.0f), b.position);
                    am = glm::rotate(am, b.selfRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

                    // Differentiate physical geometry based on scientific radar/spacecraft observations:
                    // 433 Eros: Elongated peanut shape (34x11x11 km)
                    // 4 Vesta: Oblate triaxial ellipsoid flattened by colossal south polar basin (560x560x446 km)
                    // 16 Psyche: Irregular stripped metallic protoplanet core
                    if (ai == 4) { // Eros (Peanut-shaped)
                        am = glm::rotate(am, glm::radians(25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                        am = glm::scale(am, glm::vec3(b.radius * 2.2f, b.radius * 0.95f, b.radius * 1.05f));
                    } else if (ai == 1) { // Vesta (Oblate)
                        am = glm::scale(am, glm::vec3(b.radius * 1.08f, b.radius * 0.88f, b.radius * 1.04f));
                    } else if (ai == 5) { // Psyche (Irregular metallic core)
                        am = glm::scale(am, glm::vec3(b.radius * 1.12f, b.radius * 0.92f, b.radius * 1.05f));
                    } else {
                        am = glm::scale(am, glm::vec3(b.radius));
                    }

                    // Physically motivated material properties:
                    if (ai == 5) { // 16 Psyche: Metallic nickel-iron (strong specular sheen & gloss)
                        shader.setFloat("uSpecularStrength", 0.85f);
                        shader.setFloat("uShininess", 64.0f);
                    } else if (ai == 3) { // 10 Hygiea: Primitive carbonaceous soot, albedo 0.07 (matte absorption)
                        shader.setFloat("uSpecularStrength", 0.04f);
                        shader.setFloat("uShininess", 8.0f);
                    } else if (ai == 1) { // 4 Vesta: Bright basaltic eucrite rock (high albedo 0.42)
                        shader.setFloat("uSpecularStrength", 0.35f);
                        shader.setFloat("uShininess", 32.0f);
                    } else if (ai == 0) { // 1 Ceres: Dark clay with bright sodium carbonate salt faculae
                        shader.setFloat("uSpecularStrength", 0.20f);
                        shader.setFloat("uShininess", 20.0f);
                    } else if (ai == 2) { // 2 Pallas: B-type battered golf-ball cratered rock
                        shader.setFloat("uSpecularStrength", 0.18f);
                        shader.setFloat("uShininess", 16.0f);
                    } else { // 433 Eros: Stony silicate rock
                        shader.setFloat("uSpecularStrength", 0.22f);
                        shader.setFloat("uShininess", 24.0f);
                    }

                    shader.setMat4("uModel",   am);
                    shader.setVec3("uColor",   b.color);
                    shader.setFloat("uAlpha",  1.0f);
                    if (b.texture && b.texture->hasTexture()) {
                        b.texture->bind(0); shader.setInt("uHasTexture", 1);
                    } else {
                        shader.setInt("uHasTexture", 0);
                    }
                    sphereMesh.draw();
                    stats.recordDraw(sphereMesh.indexCount());
                }
                // Reset to standard material properties
                shader.setFloat("uSpecularStrength", 0.25f);
                shader.setFloat("uShininess", 32.0f);
            }

            // ── Asteroid Belt ─────────────────────────────────────────────────
            shader.setInt("uHasTexture",0); shader.setInt("uIsEmissive",0);
            shader.setInt("uIsHighlighted",0); shader.setFloat("uAlpha",1.0f);
            asteroidBelt.draw(shader,&stats);

            // ── TRANSPARENT PASS ──────────────────────────────────────────────
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);

            // Selection highlight ONLY when manually focused in overview mode (never occlude textures in tour)
            if (inputManager.getTourState() == input::InputManager::TourState::Off)
            {
                int focusBody = inputManager.getFocusedBodyIndex();
                if (focusBody > 0 && focusBody < static_cast<int>(bodies.size()))
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow, preserves texture beneath
                    shader.use();
                    const auto& tgt = bodies[focusBody];
                    glm::mat4 shell = glm::scale(tgt.getModelMatrix(), glm::vec3(1.02f));
                    shader.setMat4("uModel", shell);
                    shader.setVec3("uColor", glm::vec3(0.0f));
                    shader.setInt("uHasTexture", 0);
                    shader.setInt("uIsHighlighted", 1);
                    shader.setVec3("uHighlightColor", glm::vec3(0.2f, 0.6f, 1.0f));
                    shader.setInt("uIsEmissive", 0);
                    shader.setFloat("uAlpha", 0.35f);
                    sphereMesh.draw();
                    stats.recordDraw(sphereMesh.indexCount());
                    shader.setInt("uIsHighlighted", 0);
                    shader.setFloat("uAlpha", 1.0f);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }

                int astFocus = inputManager.getFocusedAsteroidIndex();
                if (astFocus >= 0 && astFocus < static_cast<int>(solarSystem.getAsteroids().size()))
                {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                    shader.use();
                    const auto& ast = solarSystem.getAsteroids()[astFocus];
                    glm::mat4 shell = glm::translate(glm::mat4(1.0f), ast.body.position);
                    shell = glm::rotate(shell, ast.body.selfRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                    if (astFocus == 4) { // Eros
                        shell = glm::rotate(shell, glm::radians(25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                        shell = glm::scale(shell, glm::vec3(ast.body.radius * 2.3f, ast.body.radius * 1.05f, ast.body.radius * 1.15f));
                    } else if (astFocus == 1) { // Vesta
                        shell = glm::scale(shell, glm::vec3(ast.body.radius * 1.16f, ast.body.radius * 0.96f, ast.body.radius * 1.12f));
                    } else if (astFocus == 5) { // Psyche
                        shell = glm::scale(shell, glm::vec3(ast.body.radius * 1.20f, ast.body.radius * 1.00f, ast.body.radius * 1.13f));
                    } else {
                        shell = glm::scale(shell, glm::vec3(ast.body.radius * 1.08f));
                    }
                    shader.setMat4("uModel", shell);
                    shader.setVec3("uColor", glm::vec3(0.0f));
                    shader.setInt("uHasTexture", 0);
                    shader.setInt("uIsHighlighted", 1);
                    shader.setVec3("uHighlightColor", glm::vec3(0.3f, 0.7f, 1.0f));
                    shader.setInt("uIsEmissive", 0);
                    shader.setFloat("uAlpha", 0.35f);
                    sphereMesh.draw();
                    stats.recordDraw(sphereMesh.indexCount());
                    shader.setInt("uIsHighlighted", 0);
                    shader.setFloat("uAlpha", 1.0f);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
            }

            // Earth atmosphere
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE); glEnable(GL_CULL_FACE);
            atmoShader.use();
            atmoShader.setMat4("uView",view); atmoShader.setMat4("uProjection",proj);
            atmoShader.setVec3("uViewPos",camera.getPosition());
            atmoShader.setVec3("uColor",glm::vec3(0.2f,0.5f,1.0f));
            {
                const auto& earth=bodies[3];
                glm::mat4 ep=glm::translate(glm::mat4(1.0f),earth.position);
                glm::mat4 as=glm::scale(glm::mat4(1.0f),glm::vec3(earth.radius*1.15f));
                atmoShader.setMat4("uModel",ep*as);
                sphereMesh.draw(); stats.recordDraw(sphereMesh.indexCount());
            }
            glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

            // Saturn rings
            unlitShader.use();
            unlitShader.setMat4("uView",view); unlitShader.setMat4("uProjection",proj);
            unlitShader.setInt("uIsBillboard",0);
            unlitShader.setVec4("uColor",glm::vec4(0.85f,0.8f,0.7f,0.75f));
            glDisable(GL_CULL_FACE);
            {
                const auto& sat=bodies[6];
                glm::mat4 st=glm::translate(glm::mat4(1.0f),sat.position);
                glm::mat4 rt=glm::rotate(glm::mat4(1.0f),sat.ringTiltAngle,glm::vec3(0,0,1));
                glm::mat4 rs=glm::scale(glm::mat4(1.0f),glm::vec3(sat.radius));
                unlitShader.setMat4("uModel",st*rt*rs);
                saturnRingMesh.draw(); stats.recordDraw(saturnRingMesh.indexCount());
            }
            glEnable(GL_CULL_FACE);

            // Starfield
            unlitShader.setMat4("uModel",glm::mat4(1.0f));
            unlitShader.setVec4("uColor",glm::vec4(1,1,1,0.6f));
            glBindVertexArray(starVAO); glDrawArrays(GL_POINTS,0,800); ++stats.drawCalls;

            // Planet orbit rings
            unlitShader.setVec4("uColor",glm::vec4(1,1,1,0.15f));
            glBindVertexArray(ringVAO);
            for (size_t i=1;i<bodies.size();++i)
            {
                glm::mat4 rm=glm::scale(glm::mat4(1.0f),glm::vec3(bodies[i].orbitDistance));
                unlitShader.setMat4("uModel",rm);
                glDrawArrays(GL_LINE_LOOP,0,100); ++stats.drawCalls;
            }
            // Moon orbit ring (Earth)
            {
                const auto& earth=bodies[3];
                glm::mat4 et=glm::translate(glm::mat4(1.0f),earth.position);
                glm::mat4 mr=et*glm::scale(glm::mat4(1.0f),
                    glm::vec3(solarSystem.getMoon().orbitDistance*earth.radius));
                unlitShader.setMat4("uModel",mr);
                glDrawArrays(GL_LINE_LOOP,0,100); ++stats.drawCalls;
            }

            // Additional moon orbit rings (optional, controlled by UI toggle)
            if (uiOverlay.showMoonOrbits())
            {
                unlitShader.setVec4("uColor",glm::vec4(0.7f,0.7f,1.0f,0.10f));
                const auto& moons=solarSystem.getMoons();
                for (int mi=1;mi<static_cast<int>(moons.size());++mi)
                {
                    const auto& rec=moons[mi];
                    if (rec.parentIndex<0||rec.parentIndex>=static_cast<int>(bodies.size())) continue;
                    glm::mat4 pt=glm::translate(glm::mat4(1.0f),bodies[rec.parentIndex].position);
                    glm::mat4 ms=glm::scale(glm::mat4(1.0f),glm::vec3(rec.body.orbitDistance));
                    unlitShader.setMat4("uModel",pt*ms);
                    glDrawArrays(GL_LINE_LOOP,0,100); ++stats.drawCalls;
                }
            }

            // Comet orbit path (rough ellipse approximation as a ring)
            {
                const auto& comets=solarSystem.getComets();
                unlitShader.setVec4("uColor",glm::vec4(0.5f,0.7f,1.0f,0.12f));
                for (const auto& c : comets)
                {
                    // Draw approximated ellipse: scale ring non-uniformly (a x b)
                    float b_axis = c.semiMajor * std::sqrt(1.0f - c.eccentricity*c.eccentricity);
                    glm::mat4 orbitM = glm::mat4(1.0f);
                    orbitM = glm::rotate(orbitM, c.inclination, glm::vec3(1,0,0));
                    // Offset centre by -ae along x (one focus at origin = Sun)
                    orbitM = glm::translate(orbitM, glm::vec3(-c.semiMajor*c.eccentricity, 0, 0));
                    orbitM = orbitM * glm::scale(glm::mat4(1.0f), glm::vec3(c.semiMajor, 1.0f, b_axis));
                    unlitShader.setMat4("uModel",orbitM);
                    glDrawArrays(GL_LINE_LOOP,0,100); ++stats.drawCalls;
                }
            }

            // Named asteroid orbit rings (inclined rings in belt region)
            unlitShader.setVec4("uColor", glm::vec4(0.85f, 0.80f, 0.65f, 0.12f));
            glBindVertexArray(ringVAO);
            for (const auto& ast : solarSystem.getAsteroids())
            {
                glm::mat4 aom = glm::mat4(1.0f);
                aom = glm::rotate(aom, ast.inclination, glm::vec3(1.0f, 0.0f, 0.0f));
                aom = glm::scale(aom, glm::vec3(ast.semiMajor));
                unlitShader.setMat4("uModel", aom);
                glDrawArrays(GL_LINE_LOOP, 0, 100);
                ++stats.drawCalls;
            }

            // Comet tails (thin billboard quads, additive blend)
            if (uiOverlay.showCometTails())
            {
                glBlendFunc(GL_SRC_ALPHA,GL_ONE); // additive glow
                unlitShader.setInt("uIsBillboard",0);
                const auto& comets=solarSystem.getComets();
                for (const auto& c : comets)
                {
                    // Tail length scales with proximity to Sun (brighter/longer at perihelion)
                    // At semiMajor dist: tailLen = 2.0;  much closer: up to 8.0
                    float maxDist = c.semiMajor * (1.0f + c.eccentricity);
                    float proximity = 1.0f - (c.distFromSun / maxDist); // 0=aphelion 1=perihelion
                    proximity = std::max(0.0f, std::min(1.0f, proximity));
                    float tailLen = 2.0f + proximity * 6.0f;
                    float tailAlpha = 0.15f + proximity * 0.50f;

                    // Build rotation: local +Y → tailDir
                    glm::vec3 up = c.tailDir;
                    glm::vec3 ref = (std::abs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
                    glm::vec3 side = glm::normalize(glm::cross(ref, up));
                    glm::vec3 fwd  = glm::cross(up, side);
                    glm::mat4 tailRot(glm::vec4(side,0), glm::vec4(up,0),
                                      glm::vec4(fwd,0),  glm::vec4(0,0,0,1));

                    glm::mat4 tailM = glm::translate(glm::mat4(1.0f), c.body.position);
                    tailM = tailM * tailRot;
                    tailM = glm::scale(tailM, glm::vec3(1.0f, tailLen, 1.0f));

                    unlitShader.setMat4("uModel",tailM);
                    unlitShader.setVec4("uColor",glm::vec4(c.body.color, tailAlpha));
                    glBindVertexArray(tailVAO);
                    glDrawArrays(GL_TRIANGLES,0,6);
                    ++stats.drawCalls; stats.triangleCount+=2;
                }
                glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            }

            // Sun corona glow
            glBlendFunc(GL_SRC_ALPHA,GL_ONE);
            unlitShader.setInt("uIsBillboard",1);
            unlitShader.setVec4("uColor",glm::vec4(bodies[0].color,0.7f));
            {
                glm::mat4 gm=glm::scale(glm::mat4(1.0f),glm::vec3(bodies[0].radius*3.5f));
                unlitShader.setMat4("uModel",gm);
                glBindVertexArray(glowVAO); glDrawArrays(GL_TRIANGLES,0,6);
                ++stats.drawCalls; stats.triangleCount+=2;
            }

            glDisable(GL_BLEND); glDepthMask(GL_TRUE);

            // ── Bloom composite ───────────────────────────────────────────────
            if (bloom.isValid()) { bloom.endSceneCapture(); bloom.process(); }

            // ── Feature 1 stats ───────────────────────────────────────────────
            uiOverlay.endGpuTimer();
            uiOverlay.updateFrameStats(rawDelta*1000.0f, stats);

            // F3 edge-detect
            { static bool f3Was=false; bool f3Now=(glfwGetKey(window.handle(),GLFW_KEY_F3)==GLFW_PRESS);
              if(f3Now&&!f3Was) uiOverlay.togglePerfOverlay(); f3Was=f3Now; }

            uiOverlay.renderPanel(solarSystem, asteroidBelt, inputManager, bloom);
            uiOverlay.endFrame();

            window.swapBuffers();
            window.pollEvents();

            ++frameCount;
            if ((now-lastTime)>=1.0)
            {
                char buf[160];
                snprintf(buf,sizeof(buf),
                    "Solar System Simulator  |  %d planets  %d moons  %d asteroids  %d comets  |  %.0f FPS",
                    8, static_cast<int>(solarSystem.getMoons().size()),
                    static_cast<int>(solarSystem.getAsteroids().size()),
                    static_cast<int>(solarSystem.getComets().size()),
                    static_cast<double>(frameCount)/(now-lastTime));
                window.setTitle(buf);
                frameCount=0; lastTime=now;
            }
        }
    }
    catch (const std::exception& e)
    {
        fprintf(stderr,"\n[FATAL] %s\n",e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
