#pragma once
// =============================================================================
// CelestialBody.h — Data record for one solar system object
//
// Kept as a plain data struct so that the Sun, each planet, and any future
// moon can be described purely by data — no duplicated logic.
//
// CG concept — Scene object representation:
//   A scene is essentially a list of objects + their transforms. Here each
//   CelestialBody carries both its *identity* data (name, colour, physical
//   params) and its *transform* data (position, so the render loop can build
//   a model matrix from it with no extra state).
// =============================================================================

#include <glm/glm.hpp>
#include <string>

namespace scene {

struct CelestialBody
{
    // ── Identity / appearance ─────────────────────────────────────────────────
    std::string name;
    glm::vec3   color;          ///< RGB in [0,1] — chosen for visual distinction

    // ── Physical params (visualization scale, NOT astronomical) ──────────────
    // Real scales span ~5 orders of magnitude; we compress them so every body
    // is large enough to see and the whole system fits in one viewport.
    float radius;               ///< Visual radius in world units (SCALE)
    float orbitDistance;        ///< Distance from the Sun in world units

    // ── Animation speeds ──────────────────────────────────────────────────────
    float orbitalSpeed;         ///< Radians per second around the Sun
    float rotationSpeed;        ///< Radians per second around self Y-axis

    // ── World-space transform (set once at init, updated in render loop) ─────
    float     currentOrbitAngle;///< Current position along the orbit (radians)
    glm::vec3 position;         ///< Centre of this body in world space (TRANSLATION)
    float     selfRotationAngle;///< Rotation around the +Y axis in radians (ROTATION)

    // ── Addition 1: Saturn Rings ──────────────────────────────────────────────
    float ringInnerRadius = 0.0f;       // If 0, body has no ring
    float ringOuterRadius = 0.0f;
    float ringTiltAngle   = 0.0f;       // Fixed axial tilt (radians)

    // CG concept — Model Matrix construction:
    // Computes the local-to-world transformation matrix for this specific body.
    glm::mat4 getModelMatrix() const;
};

} // namespace scene
