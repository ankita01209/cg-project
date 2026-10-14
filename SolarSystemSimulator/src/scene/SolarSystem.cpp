// =============================================================================
// SolarSystem.cpp â€” All 9 solar system bodies, visualization-scale data
//
// IMPORTANT: sizes and distances are *deliberately compressed* for visibility.
// Real astronomical values span ~5 orders of magnitude â€” at true scale Neptune
// would be ~1,000 pixels off-screen and Mercury would be sub-pixel.
// Every number here is chosen so the whole system fits in one viewport and
// every body is clearly distinguishable.
//
// Visualization conventions:
//   - Distances: Sun=0, Mercuryâ‰ˆ4 wu, Neptuneâ‰ˆ32 wu  (wu = world units)
//   - Radii: Sun=1.5 wu (~9Ã— planet scale), gas giants~0.5, terrestrials~0.15
//   - All planets lie along the +X axis for this static phase
// =============================================================================

#include "scene/SolarSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cctype>
#include <iostream>

namespace scene {

// â”€â”€ Helper macro to reduce repetition â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
    // â”€â”€ Sun â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    // The sun simply spins in place (orbit = 0).
    { "Sun",     { 1.00f, 0.85f, 0.10f }, 1.50f,  0.0f,     0.0f,   0.2f },

    // â”€â”€ Terrestrial planets â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    { "Mercury", { 0.60f, 0.60f, 0.60f }, 0.10f,  4.0f,     1.5f,   0.3f },
    { "Venus",   { 0.90f, 0.75f, 0.40f }, 0.16f,  6.0f,     1.0f,  -0.1f }, // retrograde
    { "Earth",   { 0.20f, 0.50f, 0.90f }, 0.17f,  8.5f,     0.8f,   1.5f },
    { "Mars",    { 0.80f, 0.30f, 0.10f }, 0.12f, 11.0f,     0.6f,   1.4f },

    // â”€â”€ Gas / ice giants â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    { "Jupiter", { 0.78f, 0.60f, 0.40f }, 0.55f, 15.0f,     0.4f,   3.0f },
    { "Saturn",  { 0.90f, 0.82f, 0.55f }, 0.48f, 20.0f,     0.25f,  2.8f },
    { "Uranus",  { 0.50f, 0.82f, 0.90f }, 0.32f, 26.0f,     0.15f,- 1.8f },
    { "Neptune", { 0.20f, 0.35f, 0.90f }, 0.30f, 32.0f,     0.10f,  1.7f },
};

// =============================================================================

SolarSystem::SolarSystem()
{
    initBodies();

    // â”€â”€ Phase 7: Initialise Moon â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    m_moon.name              = "Moon";
    m_moon.color             = glm::vec3(0.5f, 0.5f, 0.5f);
    m_moon.radius            = 0.05f;
    m_moon.orbitDistance     = 0.6f;
    m_moon.orbitalSpeed      = 2.0f;
    m_moon.rotationSpeed     = 0.8f;
    m_moon.currentOrbitAngle = 0.0f;
    m_moon.selfRotationAngle = 0.0f;
    m_moon.position          = glm::vec3(m_moon.orbitDistance, 0.0f, 0.0f);

    // â”€â”€ Addition 1: Saturn Rings â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

    // â”€â”€ Pre-process automatic texture caching â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    int loadedTexCount = 0;
    for (auto& body : m_bodies)
    {
        std::string lowerName = body.name;
        for (auto& c : lowerName) c = static_cast<char>(std::tolower(c));

        std::string jpgPath = std::string(ASSETS_DIR) + "textures/planets/" + lowerName + ".jpg";
        body.texture = loadTexture(jpgPath);
        if (!body.texture->hasTexture()) {
            body.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/" + lowerName + ".png");
        }
        if (body.texture->hasTexture()) loadedTexCount++;
    }

    // Cache the Moon
    std::string moonJpg = std::string(ASSETS_DIR) + "textures/planets/moon.jpg";
    m_moon.texture = loadTexture(moonJpg);
    if (!m_moon.texture->hasTexture()) {
        m_moon.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/moon.png");
    }
    if (m_moon.texture->hasTexture()) loadedTexCount++;

    std::cout << "[SolarSystem] Successfully cached " << loadedTexCount << " / " << (m_bodies.size() + 1) << " surface textures." << std::endl;
}

render::Texture* SolarSystem::loadTexture(const std::string& path)
{
    if (m_textureCache.find(path) == m_textureCache.end())
    {
        auto tex = std::make_unique<render::Texture>();
        tex->load(path);
        m_textureCache[path] = std::move(tex);
    }
    return m_textureCache[path].get();
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
// CG concept â€” Scene Graph Hierarchy:
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
