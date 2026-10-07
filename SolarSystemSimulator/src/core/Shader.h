#pragma once
// =============================================================================
// Shader.h — Loads, compiles, links, and uses a GLSL shader program
//
// CG concept: A "shader program" is a small GPU program.  A vertex shader
// runs once per vertex (transforms positions) and a fragment shader runs
// once per rasterised pixel (computes the final colour).  Together they form
// the programmable part of the OpenGL rendering pipeline.
// =============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

namespace core {

class Shader
{
public:
    /// Load, compile, and link vertex + fragment shader source files.
    /// Prints detailed error messages and throws std::runtime_error on failure.
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Non-copyable (GPU program IDs cannot be trivially duplicated)
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;

    /// Bind this shader program for subsequent draw calls.
    void use() const;

    /// GPU program handle (rarely needed directly)
    GLuint id() const { return m_programID; }

    // ── Uniform setters ───────────────────────────────────────────────────────
    // CG concept: uniforms are variables sent from the CPU to the GPU
    // shaders every frame (e.g. the model/view/projection matrices).
    void setInt  (const std::string& name, int   value)          const;
    void setFloat(const std::string& name, float value)          const;
    void setVec3 (const std::string& name, const glm::vec3& v)   const;
    void setVec4 (const std::string& name, const glm::vec4& v)   const;
    void setMat4 (const std::string& name, const glm::mat4& mat) const;

private:
    GLuint m_programID = 0;

    // Reads an entire file into a string.
    static std::string readFile(const std::string& path);

    // Compiles a single shader stage; prints errors; returns the shader ID.
    static GLuint compileStage(GLenum type, const std::string& source,
                               const std::string& debugName);
};

} // namespace core
