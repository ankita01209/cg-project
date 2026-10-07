// =============================================================================
// Shader.cpp — GLSL shader compilation, linking, and uniform helpers
// =============================================================================

#include "core/Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstdio>

namespace core {

// ── Construction / Destruction ────────────────────────────────────────────────

Shader::Shader(const std::string& vertPath, const std::string& fragPath)
{
    // Read source files from disk
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);

    // ── Compile individual stages ─────────────────────────────────────────────
    // CG concept – Shader stages:
    //   GL_VERTEX_SHADER   : runs per-vertex (geometry transformation)
    //   GL_FRAGMENT_SHADER : runs per-fragment (pixel colour computation)
    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertSrc, vertPath);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc, fragPath);

    // ── Link into a shader program ────────────────────────────────────────────
    // CG concept – Shader program:
    //   Linking combines compiled stages, resolves the connection between
    //   vertex output varyings and fragment input varyings, and validates
    //   the whole pipeline as a unit.
    m_programID = glCreateProgram();
    glAttachShader(m_programID, vert);
    glAttachShader(m_programID, frag);
    glLinkProgram(m_programID);

    // Check for link errors
    int  success = 0;
    char infoLog[1024];
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_programID, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(vert);
        glDeleteShader(frag);
        glDeleteProgram(m_programID);
        m_programID = 0;
        throw std::runtime_error(
            std::string("[Shader] Link error (") + vertPath + " + " + fragPath + "):\n" + infoLog);
    }

    // Individual stage objects are no longer needed after linking
    glDeleteShader(vert);
    glDeleteShader(frag);

    printf("[Shader] Program linked OK (vert=%s, frag=%s)\n",
           vertPath.c_str(), fragPath.c_str());
}

Shader::~Shader()
{
    if (m_programID)
        glDeleteProgram(m_programID);
}

// ── Runtime methods ───────────────────────────────────────────────────────────

void Shader::use() const
{
    glUseProgram(m_programID);
}

// ── Uniform setters (query location each call — fine for a student project) ───

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_programID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_programID, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& v) const
{
    glUniform3fv(glGetUniformLocation(m_programID, name.c_str()), 1, glm::value_ptr(v));
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const
{
    glUniform4fv(glGetUniformLocation(m_programID, name.c_str()), 1, glm::value_ptr(v));
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
    // GL_FALSE = column-major (GLM's default storage order matches OpenGL's)
    glUniformMatrix4fv(
        glGetUniformLocation(m_programID, name.c_str()),
        1, GL_FALSE, glm::value_ptr(mat));
}

// ── Private helpers ───────────────────────────────────────────────────────────

std::string Shader::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("[Shader] Cannot open file: " + path);

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compileStage(GLenum type, const std::string& source,
                             const std::string& debugName)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // ── Compilation error reporting ───────────────────────────────────────────
    // CG concept – Compile errors are GPU-side; the driver returns them as a
    // plain text "info log" that names the offending line number and reason.
    int  success = 0;
    char infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(shader);
        const char* typeName = (type == GL_VERTEX_SHADER) ? "Vertex" : "Fragment";
        throw std::runtime_error(
            std::string("[Shader] ") + typeName + " compile error (" + debugName + "):\n" + infoLog);
    }

    return shader;
}

} // namespace core
