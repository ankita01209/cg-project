#pragma once
// =============================================================================
// Mesh.h — GPU-resident vertex/index buffer wrapper
//
// CG concept — Indexed geometry:
//   A sphere shares many vertices between adjacent triangles. Storing each
//   vertex once and referencing it by index (via an EBO / Index Buffer) saves
//   memory and lets the GPU cache vertices efficiently.
//
// Layout: every vertex is 8 floats = 32 bytes
//   [0..2]  position  (vec3)
//   [3..5]  normal    (vec3)   ← used in Phase 3 to visualise 3D shape
//   [6..7]  texCoord  (vec2)   ← reserved for texturing in a later phase
// =============================================================================

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace render {

// Interleaved vertex type — matches the VAO attribute description below
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh
{
public:
    /// Upload vertices + indices to the GPU and configure the VAO.
    Mesh(const std::vector<Vertex>& vertices,
         const std::vector<unsigned int>& indices);

    ~Mesh();

    // Move-only: GPU buffer handles cannot be trivially copied
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    /// Issue the draw call for this mesh (requires a shader to be bound first)
    void draw() const;
    void drawInstanced(int count) const; // Phase 1: Asteroid belt GPU instancing

    int indexCount()  const { return m_indexCount;  }
    int vertexCount() const { return m_vertexCount; }
    GLuint getVAO()   const { return m_vao; } // Phase 1: Expose VAO for attaching instance buffers
    GLuint m_vao        = 0;
    GLuint m_vbo        = 0;
    GLuint m_ebo        = 0;
    int    m_indexCount = 0;
    int    m_vertexCount= 0;

    void upload(const std::vector<Vertex>& vertices,
                const std::vector<unsigned int>& indices);
};

} // namespace render
