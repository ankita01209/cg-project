// =============================================================================
// SolarSystem.cpp — All 9 solar system bodies, visualization-scale data
//
// IMPORTANT: sizes and distances are *deliberately compressed* for visibility.
// Real astronomical values span ~5 orders of magnitude — at true scale Neptune
// would be ~1,000 pixels off-screen and Mercury would be sub-pixel.
// Every number here is chosen so the whole system fits in one viewport and
// every body is clearly distinguishable.
//
// Visualization conventions:
//   - Distances: Sun=0, Mercury≈4 wu, Neptune≈32 wu  (wu = world units)
//   - Radii: Sun=1.5 wu (~9× planet scale), gas giants~0.5, terrestrials~0.15
//   - All planets lie along the +X axis for this static phase
// =============================================================================

#include "scene/SolarSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

namespace scene {

// ── Helper macro to reduce repetition ────────────────────────────────────────
// Each row: { name, {R,G,B}, visualRadius, orbitDistance }
// Position is computed below (orbit along +X axis).
struct BodyDef {
    const char* name;
    glm::vec3   color;
    float       radius;
    float       orbitDist;
    float       orbitSpeed;
    float       rotationSpeed;
};

// =============================================================================
// Visualization-scale dataset
//
// Column guide:  name        colour (R,G,B)          vis-radius  orbit-dist  orbit-spd  rot-spd
// =============================================================================
static const BodyDef k_bodyDefs[] = {
    // ── Sun ──────────────────────────────────────────────────────────────────
    // The sun simply spins in place (orbit = 0).
    { "Sun",     { 1.00f, 0.85f, 0.10f }, 1.50f,  0.0f,     0.0f,   0.2f },

    // ── Terrestrial planets ───────────────────────────────────────────────────
    { "Mercury", { 0.60f, 0.60f, 0.60f }, 0.10f,  4.0f,     1.5f,   0.3f },
    { "Venus",   { 0.90f, 0.75f, 0.40f }, 0.16f,  6.0f,     1.0f,  -0.1f }, // retrograde
    { "Earth",   { 0.20f, 0.50f, 0.90f }, 0.17f,  8.5f,     0.8f,   1.5f },
    { "Mars",    { 0.80f, 0.30f, 0.10f }, 0.12f, 11.0f,     0.6f,   1.4f },

    // ── Gas / ice giants ──────────────────────────────────────────────────────
    { "Jupiter", { 0.78f, 0.60f, 0.40f }, 0.55f, 15.0f,     0.4f,   3.0f },
    { "Saturn",  { 0.90f, 0.82f, 0.55f }, 0.48f, 20.0f,     0.25f,  2.8f },
    { "Uranus",  { 0.50f, 0.82f, 0.90f }, 0.32f, 26.0f,     0.15f,- 1.8f },
    { "Neptune", { 0.20f, 0.35f, 0.90f }, 0.30f, 32.0f,     0.10f,  1.7f },
};

// =============================================================================

SolarSystem::SolarSystem()
{
    initBodies();

    // ── Phase 7: Hierarchy (Moon) ──────────────────────────────────────────
    m_moon.name              = "Moon";
    m_moon.color             = glm::vec3(0.7f, 0.7f, 0.75f);
    // Remember: Moon's radius/orbit will be scaled by Earth's scale (0.17) dynamically in getMoonModelMatrix.
    // So to make it visibile (e.g. final world radius 0.05), we need local radius ~ 0.3.
    // To make orbit distance ~2.0 from Earth in world space, local distance = 2.0 / 0.17 ~ 12.0.
    m_moon.radius            = 0.3f;  
    m_moon.orbitDistance     = 12.0f;
    m_moon.orbitalSpeed      = 4.0f;  // Orbits Earth quickly
    m_moon.rotationSpeed     = 1.0f;
    m_moon.currentOrbitAngle = 0.0f;
    m_moon.selfRotationAngle = 0.0f;
    m_moon.position          = glm::vec3(m_moon.orbitDistance, 0.0f, 0.0f);
    
    // ── Addition 1: Saturn Rings ──────────────────────────────────────────────
    m_bodies[6].ringInnerRadius = 1.3f; 
    m_bodies[6].ringOuterRadius = 2.5f;
    m_bodies[6].ringTiltAngle   = glm::radians(26.7f);
}

void SolarSystem::initBodies()
{
    m_bodies.clear();
    m_bodies.reserve(9);

    for (const auto& def : k_bodyDefs)
    {
        CelestialBody body;
        body.name         = def.name;
        body.color        = def.color;
        body.radius       = def.radius;
        body.orbitDistance= def.orbitDist;

        body.orbitalSpeed     = def.orbitSpeed;
        body.rotationSpeed    = def.rotationSpeed;
        body.currentOrbitAngle= 0.0f;
        body.selfRotationAngle= 0.0f;

        // Initial position (angle = 0 -> along +X axis)
        body.position = glm::vec3(def.orbitDist, 0.0f, 0.0f);

        m_bodies.push_back(body);

        printf("[SolarSystem] %-8s  radius=%.2f  orbit=%.1f  spd=%.2f\n",
               def.name, def.radius, def.orbitDist, def.orbitSpeed);
    }
}

void SolarSystem::update(float deltaTime)
{
    // Phase 6: Animate all planets continuously based on their individual speeds.
    for (auto& body : m_bodies)
    {
        // 1. Accumulate angles based on distinct speeds
        body.currentOrbitAngle += body.orbitalSpeed  * deltaTime;
        body.selfRotationAngle += body.rotationSpeed * deltaTime;

        // 2. Recompute X,Z position (orbits in the XZ plane)
        // CG concept: Circular orbit path using sine/cosine
        body.position.x = std::cos(body.currentOrbitAngle) * body.orbitDistance;
        body.position.z = std::sin(body.currentOrbitAngle) * body.orbitDistance;
    }

    // Phase 7: Update Moon
    m_moon.currentOrbitAngle += m_moon.orbitalSpeed  * deltaTime;
    m_moon.selfRotationAngle += m_moon.rotationSpeed * deltaTime;
    m_moon.position.x = std::cos(m_moon.currentOrbitAngle) * m_moon.orbitDistance;
    m_moon.position.z = std::sin(m_moon.currentOrbitAngle) * m_moon.orbitDistance;
}

// =============================================================================
// Phase 7: Hierarchical Transformation
// =============================================================================
// CG concept — Scene Graph Hierarchy:
//   The Moon revolves around the Earth, not the Sun. To find its final world
//   position, we don't just place it in space. We compute its local transform
//   relative to Earth, then multiply it by Earth's world transform.
//   EarthWorldTransform * MoonLocalOrbitTransform * MoonSpin
glm::mat4 SolarSystem::getMoonModelMatrix() const
{
    // Earth is at index 3 in k_bodyDefs (Sun=0, Mercury=1, Venus=2, Earth=3)
    const CelestialBody& earth = m_bodies[3];

    // Earth's world transform
    glm::mat4 earthWorld = earth.getModelMatrix();

    // Moon's local orbit transform (translation relative to Earth)
    glm::mat4 moonOrbit = glm::translate(glm::mat4(1.0f), m_moon.position);

    // Moon's local spin (and scale)
    glm::mat4 moonSpinScale = glm::rotate(glm::mat4(1.0f), m_moon.selfRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    moonSpinScale = glm::scale(moonSpinScale, glm::vec3(m_moon.radius));

    // Combine: Parent * Child's Local Transform
    // Note: Because earthWorld includes Earth's scale (0.17), moonOrbit distance and 
    // moonSpinScale will implicitly be scaled down by 0.17. We accounted for this in initialization.
    return earthWorld * moonOrbit * moonSpinScale;
}

} // namespace scene
