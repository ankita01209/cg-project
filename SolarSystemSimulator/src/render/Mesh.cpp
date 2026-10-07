// =============================================================================
// Mesh.cpp — VAO / VBO / EBO upload and draw
// =============================================================================

#include "render/Mesh.h"
#include <utility>   // std::move, std::swap

namespace render {

// ── Upload ────────────────────────────────────────────────────────────────────

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<unsigned int>& indices)
{
    upload(vertices, indices);
}

void Mesh::upload(const std::vector<Vertex>& vertices,
                  const std::vector<unsigned int>& indices)
{
    m_vertexCount = static_cast<int>(vertices.size());
    m_indexCount  = static_cast<int>(indices.size());

    // ── Generate GPU objects ──────────────────────────────────────────────────
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // CG concept — VBO: upload raw vertex bytes to GPU VRAM
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    // CG concept — EBO (Element Buffer Object / Index Buffer):
    //   Stores a list of vertex indices. glDrawElements reads this list so
    //   the same vertex can be referenced by multiple triangles, cutting
    //   memory use roughly in half compared to storing duplicates.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    // ── Vertex attribute descriptions (must match the Vertex struct layout) ───
    constexpr GLsizei stride = sizeof(Vertex);   // 32 bytes

    // Location 0: position  (vec3, offset 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, position)));

    // Location 1: normal  (vec3, offset 12)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, normal)));

    // Location 2: texCoord  (vec2, offset 24)  — reserved for texturing
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void*>(offsetof(Vertex, texCoord)));

    glBindVertexArray(0);   // unbind VAO — good practice
}

// ── Move semantics ────────────────────────────────────────────────────────────

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo)
    , m_indexCount(other.m_indexCount), m_vertexCount(other.m_vertexCount)
{
    // Null the source so its destructor won't double-free GPU objects
    other.m_vao = other.m_vbo = other.m_ebo = 0;
    other.m_indexCount = other.m_vertexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        // Release current GPU resources
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_ebo) glDeleteBuffers(1, &m_ebo);

        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount; m_vertexCount = other.m_vertexCount;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
        other.m_indexCount = other.m_vertexCount = 0;
    }
    return *this;
}

// ── Destructor ────────────────────────────────────────────────────────────────

Mesh::~Mesh()
{
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ebo) glDeleteBuffers(1, &m_ebo);
}

// ── Draw ──────────────────────────────────────────────────────────────────────

void Mesh::draw() const
{
    glBindVertexArray(m_vao);
    // CG concept — Indexed draw call:
    //   glDrawElements walks the EBO and feeds the referenced vertices into
    //   the vertex shader. GL_UNSIGNED_INT matches our index type.
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(m_indexCount),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace render
