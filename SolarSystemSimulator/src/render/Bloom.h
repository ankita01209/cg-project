#pragma once
// =============================================================================
// Bloom.h  —  HDR Framebuffer Capture + Separable Gaussian Bloom (Feature 2)
//
// Pipeline (called once per frame in main.cpp):
//
//   bloom.resize(w, h);              // on first frame or after window resize
//   bloom.beginSceneCapture();       // bind HDR FBO; clear
//   /* ... render entire 3-D scene ... */
//   bloom.endSceneCapture();         // back to default FBO
//   bloom.process();                 // bright-pass → blur → composite → default FB
//   /* ... render ImGui on top ... */
//
// All OpenGL objects (FBOs, textures, RBOs) are owned by this class (RAII).
// Window resize is handled safely: call resize() whenever framebuffer changes.
//
// Public fields (intensity, threshold, exposure, blurPasses, enabled) are read
// every frame by process() and written by the ImGui Observatory controls.
// =============================================================================

#include <glad/glad.h>
#include <string>
#include <memory>

namespace core { class Shader; }  // forward declaration (avoids pulling in Shader.h here)

namespace render {

class Bloom
{
public:
    // ── Per-frame settings (set from UIOverlay Observatory tab) ──────────────
    bool  enabled    = true;
    float intensity  = 0.6f;  ///< Additive bloom strength
    float threshold  = 0.8f;  ///< Luminance threshold for bright-pass (Rec.709)
    float exposure   = 1.0f;  ///< Reinhard tone-mapping exposure
    int   blurPasses = 5;     ///< Number of H+V Gaussian blur iterations

    /// @param assetDir  Absolute path ending with '/' (matches ASSETS_DIR macro)
    explicit Bloom(const std::string& assetDir);
    ~Bloom();

    Bloom(const Bloom&)            = delete;
    Bloom& operator=(const Bloom&) = delete;

    /// (Re)create all FBOs at the given framebuffer size.
    /// Safe to call every frame — returns immediately if size unchanged.
    void resize(int width, int height);

    /// Bind the HDR FBO and clear it. Call immediately before the 3-D render.
    void beginSceneCapture();

    /// Unbind the HDR FBO (back to default). Call immediately after the 3-D render.
    void endSceneCapture();

    /// Run bright-pass → Gaussian blur → composite to the DEFAULT framebuffer.
    /// Must be called after endSceneCapture() and BEFORE ImGui rendering.
    void process();

    /// Returns false if shader compilation failed; main.cpp falls back to LDR.
    bool isValid() const { return m_hdrFBO != 0; }

private:
    void createBuffers(int width, int height);
    void destroyBuffers();
    void drawQuad();

    int m_width  = 0;
    int m_height = 0;

    // ── HDR scene capture (full-resolution RGBA16F) ───────────────────────────
    GLuint m_hdrFBO      = 0;
    GLuint m_hdrColorTex = 0;
    GLuint m_hdrDepthRBO = 0;

    // ── Bright-pass result (half-resolution RGBA16F) ──────────────────────────
    GLuint m_brightFBO = 0;
    GLuint m_brightTex = 0;

    // ── Ping-pong blur buffers (two half-resolution RGBA16F FBOs) ─────────────
    GLuint m_pingFBO[2] = {0, 0};
    GLuint m_pingTex[2] = {0, 0};

    // ── Screen-space quad ────────────────────────────────────────────────────
    GLuint m_quadVAO = 0;
    GLuint m_quadVBO = 0;

    // ── Post-processing shaders (unique_ptr so Shader's no-default-ctor is OK) ─
    std::unique_ptr<core::Shader> m_shaderBright;     ///< Bright-pass extraction
    std::unique_ptr<core::Shader> m_shaderBlur;       ///< Separable Gaussian blur
    std::unique_ptr<core::Shader> m_shaderComposite;  ///< Tone-map + bloom add
};

} // namespace render
