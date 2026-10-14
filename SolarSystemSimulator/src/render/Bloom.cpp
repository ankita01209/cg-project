// =============================================================================
// Bloom.cpp  —  HDR Framebuffer + Separable Gaussian Bloom Post-Processing
// =============================================================================

#include "render/Bloom.h"
#include "core/Shader.h"

#include <glad/glad.h>
#include <algorithm>
#include <cstdio>

namespace render {

// Screen-space quad in NDC: two triangles, attribute layout = (x, y, u, v)
static const float QUAD_VERTS[] = {
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f,
    -1.0f,  1.0f,  0.0f, 1.0f
};

// =============================================================================
// Construction / Destruction
// =============================================================================

Bloom::Bloom(const std::string& assetDir)
{
    const std::string vert = assetDir + "shaders/fullscreen.vert";

    m_shaderBright    = std::make_unique<core::Shader>(
        vert, assetDir + "shaders/bloom_brightness.frag");
    m_shaderBlur      = std::make_unique<core::Shader>(
        vert, assetDir + "shaders/bloom_blur.frag");
    m_shaderComposite = std::make_unique<core::Shader>(
        vert, assetDir + "shaders/bloom_composite.frag");

    // Screen-space quad VAO / VBO
    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);

    glBindVertexArray(m_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_VERTS), QUAD_VERTS, GL_STATIC_DRAW);

    // location 0 = position (xy)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)0);
    // location 1 = texcoord (uv)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);

    printf("[Bloom] Post-processing shaders loaded OK.\n");
}

Bloom::~Bloom()
{
    destroyBuffers();
    if (m_quadVAO) { glDeleteVertexArrays(1, &m_quadVAO); m_quadVAO = 0; }
    if (m_quadVBO) { glDeleteBuffers(1, &m_quadVBO);      m_quadVBO = 0; }
}

// =============================================================================
// Resize (re-create all FBOs whenever window dimensions change)
// =============================================================================

void Bloom::resize(int width, int height)
{
    if (width == m_width && height == m_height) return;
    if (width <= 0 || height <= 0) return;

    m_width  = width;
    m_height = height;
    destroyBuffers();
    createBuffers(width, height);

    printf("[Bloom] FBOs resized: %dx%d  (half-res blur: %dx%d)\n",
           width, height, width / 2, height / 2);
}

// =============================================================================
// FBO helpers
// =============================================================================

static GLuint makeColorTex(int w, int h)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
                 GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

void Bloom::createBuffers(int width, int height)
{
    const int hw = std::max(1, width  / 2);
    const int hh = std::max(1, height / 2);

    // ── HDR scene FBO (full-resolution RGBA16F + depth RBO) ──────────────────
    glGenFramebuffers(1, &m_hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

    m_hdrColorTex = makeColorTex(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_hdrColorTex, 0);

    glGenRenderbuffers(1, &m_hdrDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, m_hdrDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, m_hdrDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("[Bloom] WARNING: HDR FBO incomplete!\n");

    // ── Bright-pass FBO (half-resolution) ─────────────────────────────────────
    glGenFramebuffers(1, &m_brightFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
    m_brightTex = makeColorTex(hw, hh);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, m_brightTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        printf("[Bloom] WARNING: Bright-pass FBO incomplete!\n");

    // ── Ping-pong blur FBOs (two half-resolution buffers) ─────────────────────
    glGenFramebuffers(2, m_pingFBO);
    glGenTextures(2, m_pingTex);
    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingFBO[i]);
        m_pingTex[i] = makeColorTex(hw, hh);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, m_pingTex[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            printf("[Bloom] WARNING: Ping-pong FBO %d incomplete!\n", i);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Bloom::destroyBuffers()
{
    if (m_hdrFBO)       { glDeleteFramebuffers(1, &m_hdrFBO);       m_hdrFBO      = 0; }
    if (m_hdrColorTex)  { glDeleteTextures(1, &m_hdrColorTex);      m_hdrColorTex = 0; }
    if (m_hdrDepthRBO)  { glDeleteRenderbuffers(1, &m_hdrDepthRBO); m_hdrDepthRBO = 0; }
    if (m_brightFBO)    { glDeleteFramebuffers(1, &m_brightFBO);    m_brightFBO   = 0; }
    if (m_brightTex)    { glDeleteTextures(1, &m_brightTex);        m_brightTex   = 0; }

    if (m_pingFBO[0]) { glDeleteFramebuffers(2, m_pingFBO); m_pingFBO[0] = m_pingFBO[1] = 0; }
    if (m_pingTex[0]) { glDeleteTextures(2, m_pingTex);     m_pingTex[0] = m_pingTex[1] = 0; }
}

// =============================================================================
// Per-frame render pipeline
// =============================================================================

void Bloom::beginSceneCapture()
{
    if (m_hdrFBO == 0 || m_width == 0) return;
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // deep space background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Bloom::endSceneCapture()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Bloom::drawQuad()
{
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Bloom::process()
{
    if (m_hdrFBO == 0 || m_width == 0) return;

    const int hw = std::max(1, m_width  / 2);
    const int hh = std::max(1, m_height / 2);

    // Save and override state for post-process passes
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    // ── Pass 1: Bright-pass extraction (full-res → half-res) ─────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, m_brightFBO);
    glViewport(0, 0, hw, hh);
    glClear(GL_COLOR_BUFFER_BIT);

    m_shaderBright->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrColorTex);
    m_shaderBright->setInt("uHdrScene",  0);
    m_shaderBright->setFloat("uThreshold", threshold);
    drawQuad();

    // ── Pass 2: Separable Gaussian blur (ping-pong, blurPasses iterations) ───
    // Each iteration = one horizontal pass + one vertical pass.
    // We start by reading from m_brightTex, then alternate between m_pingTex[0/1].
    GLuint srcTex   = m_brightTex;
    bool   horizontal = true;

    m_shaderBlur->use();
    m_shaderBlur->setInt("uImage", 0);

    const int totalPasses = std::max(1, blurPasses) * 2;
    for (int pass = 0; pass < totalPasses; ++pass)
    {
        const int dst = pass & 1;   // alternates 0, 1, 0, 1, ...
        glBindFramebuffer(GL_FRAMEBUFFER, m_pingFBO[dst]);
        glViewport(0, 0, hw, hh);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTex);
        m_shaderBlur->setInt("uHorizontal", horizontal ? 1 : 0);
        drawQuad();

        srcTex     = m_pingTex[dst];
        horizontal = !horizontal;
    }
    // srcTex now points to the last-written ping-pong texture (the blur result)
    const GLuint blurResult = srcTex;

    // ── Pass 3: Composite to default framebuffer ──────────────────────────────
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT);  // clear default FB before compositing

    m_shaderComposite->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrColorTex);
    m_shaderComposite->setInt("uHdrScene", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurResult);
    m_shaderComposite->setInt("uBloom", 1);

    m_shaderComposite->setFloat("uExposure",       exposure);
    m_shaderComposite->setFloat("uBloomIntensity", intensity);
    m_shaderComposite->setInt("uBloomEnabled",     enabled ? 1 : 0);
    drawQuad();

    // Restore OpenGL state for subsequent passes (ImGui etc.)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glActiveTexture(GL_TEXTURE0); // leave unit 0 active by convention
}

} // namespace render
