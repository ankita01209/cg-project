// =============================================================================
// Sphere.cpp — Procedural UV-sphere geometry generator
// =============================================================================

#include "render/Sphere.h"

#include <glm/gtc/constants.hpp>
#include <cmath>

namespace render {

// =============================================================================
// createSphere
//
// CG concept — UV-sphere (latitude/longitude sphere):
//   We lay a regular grid of (stacks+1) × (slices+1) sample points over the
//   sphere surface.  Latitude "φ" (phi) and longitude "θ" (theta) are the
//   two parameters:
//
//     phi   ∈ [0, π]     top pole (0) → bottom pole (π)
//     theta ∈ [0, 2π]    left edge → around → right edge (same as left)
//
//   Parametric equations (unit sphere, scaled by radius):
//     x = radius · sin(phi) · cos(theta)
//     y = radius · cos(phi)              ← Y is "up"
//     z = radius · sin(phi) · sin(theta)
//
//   The outward surface normal is just the normalised position vector:
//     normal = vec3(x, y, z) / radius
//
//   UV texture coordinates:
//     u = theta / (2π)     ← wraps 0→1 left to right
//     v = phi   / π        ← 0 at top pole, 1 at bottom
//
// Indexing:
//   Adjacent sample points form quads, each split into 2 triangles.
//   Winding order: counter-clockwise when viewed from outside (OpenGL default).
// =============================================================================

Mesh createSphere(float radius, int stacks, int slices)
{
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(static_cast<size_t>((stacks + 1) * (slices + 1)));

    const float pi    = glm::pi<float>();
    const float twoPi = glm::two_pi<float>();

    // ── Generate vertices ──────────────────────────────────────────────────────
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = static_cast<float>(i) * pi / static_cast<float>(stacks);
        float sinP = std::sin(phi);
        float cosP = std::cos(phi);

        for (int j = 0; j <= slices; ++j)
        {
            float theta = static_cast<float>(j) * twoPi / static_cast<float>(slices);
            float sinT  = std::sin(theta);
            float cosT  = std::cos(theta);

            // Unit-sphere position (before radius scaling)
            glm::vec3 unitPos(sinP * cosT,   // x
                              cosP,           // y = "up" axis
                              sinP * sinT);   // z

            Vertex v;
            v.position = unitPos * radius;
            v.normal   = unitPos;             // unit outward normal
            v.texCoord = glm::vec2(
                static_cast<float>(j) / static_cast<float>(slices),  // u
                static_cast<float>(i) / static_cast<float>(stacks)   // v
            );

            vertices.push_back(v);
        }
    }

    // ── Generate indices (two triangles per quad) ──────────────────────────────
    // For stack i, slice j:
    //   top-left  = i      * (slices+1) + j
    //   top-right = i      * (slices+1) + j+1
    //   bot-left  = (i+1)  * (slices+1) + j
    //   bot-right = (i+1)  * (slices+1) + j+1
    //
    // Triangle 1: top-left, bot-left, bot-right   (CCW from outside)
    // Triangle 2: top-left, bot-right, top-right

    indices.reserve(static_cast<size_t>(stacks * slices * 6));
    int rowLen = slices + 1;

    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            unsigned int tl = static_cast<unsigned int>(i       * rowLen + j);
            unsigned int tr = static_cast<unsigned int>(i       * rowLen + j + 1);
            unsigned int bl = static_cast<unsigned int>((i + 1) * rowLen + j);
            unsigned int br = static_cast<unsigned int>((i + 1) * rowLen + j + 1);

            // Triangle 1 (upper-left of quad)
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(br);

            // Triangle 2 (lower-right of quad)
            indices.push_back(tl);
            indices.push_back(br);
            indices.push_back(tr);
        }
    }

    return Mesh(vertices, indices);
}

// =============================================================================
// Addition 1: Saturn Rings - Procedural Annulus
// =============================================================================
Mesh createRing(float innerRadius, float outerRadius, int slices)
{
    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    const float twoPi = glm::two_pi<float>();
    
    // Generate slice+1 rings of 2 vertices (inner, outer)
    for (int i = 0; i <= slices; ++i)
    {
        float theta = static_cast<float>(i) * twoPi / static_cast<float>(slices);
        float texU  = static_cast<float>(i) / static_cast<float>(slices); // u wraps 0->1
        
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);
        
        // Inner radius vertex
        Vertex inV;
        inV.position = glm::vec3(innerRadius * cosT, 0.0f, innerRadius * sinT);
        inV.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
        inV.texCoord = glm::vec2(texU, 0.0f);
        vertices.push_back(inV);
        
        // Outer radius vertex
        Vertex outV;
        outV.position = glm::vec3(outerRadius * cosT, 0.0f, outerRadius * sinT);
        outV.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
        outV.texCoord = glm::vec2(texU, 1.0f);
        vertices.push_back(outV);
    }
    
    // Generate indices (quads split into 2 triangles)
    for (int i = 0; i < slices; ++i)
    {
        unsigned int in1  = static_cast<unsigned int>(i * 2);
        unsigned int out1 = static_cast<unsigned int>(i * 2 + 1);
        unsigned int in2  = static_cast<unsigned int>((i + 1) * 2);
        unsigned int out2 = static_cast<unsigned int>((i + 1) * 2 + 1);
        
        // Triangle 1 (CCW)
        indices.push_back(in1);
        indices.push_back(in2);
        indices.push_back(out1);
        
        // Triangle 2 (CCW)
        indices.push_back(out1);
        indices.push_back(in2);
        indices.push_back(out2);
    }
    
    return Mesh(vertices, indices);
}

} // namespace render
