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
};

// =============================================================================
// Visualization-scale dataset
//
// Column guide:  name        colour (R,G,B)          vis-radius  orbit-dist
// =============================================================================
static const BodyDef k_bodyDefs[] = {
    // ── Sun ──────────────────────────────────────────────────────────────────
    // Real: ~696,000 km radius. Vis: 1.5 units — large but not screen-filling.
    { "Sun",     { 1.00f, 0.85f, 0.10f }, 1.50f,  0.0f  },

    // ── Terrestrial planets ───────────────────────────────────────────────────
    // Real orbits: 0.39–1.52 AU → compressed to 4–11 units.
    // Real radii: 2,440–6,371 km → all shown at 0.10–0.17 for clarity.
    { "Mercury", { 0.60f, 0.60f, 0.60f }, 0.10f,  4.0f  }, // gray
    { "Venus",   { 0.90f, 0.75f, 0.40f }, 0.16f,  6.0f  }, // pale amber
    { "Earth",   { 0.20f, 0.50f, 0.90f }, 0.17f,  8.5f  }, // ocean blue
    { "Mars",    { 0.80f, 0.30f, 0.10f }, 0.12f, 11.0f  }, // rust red

    // ── Gas / ice giants ──────────────────────────────────────────────────────
    // Real orbits: 5.2–30 AU → compressed to 15–32 units.
    // Real radii: 25,560–71,490 km → shown 0.30–0.55 (larger than terrestrials).
    { "Jupiter", { 0.78f, 0.60f, 0.40f }, 0.55f, 15.0f  }, // tan/orange
    { "Saturn",  { 0.90f, 0.82f, 0.55f }, 0.48f, 20.0f  }, // pale gold
    { "Uranus",  { 0.50f, 0.82f, 0.90f }, 0.32f, 26.0f  }, // ice cyan
    { "Neptune", { 0.20f, 0.35f, 0.90f }, 0.30f, 32.0f  }, // deep blue
};

// =============================================================================

SolarSystem::SolarSystem()
{
    initBodies();
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

        // CG concept — Model transform / scene placement:
        //   We place every body along the world +X axis at its orbital distance.
        //   The Sun is at the origin; planets fan out to the right.
        //   In a later phase this static position will be replaced by an angle-
        //   driven orbit: position = (cos(angle)*dist, 0, sin(angle)*dist).
        body.position = glm::vec3(def.orbitDist, 0.0f, 0.0f);

        m_bodies.push_back(body);

        printf("[SolarSystem] %-8s  radius=%.2f  orbit=%.1f  pos=(%.1f,0,0)\n",
               def.name, def.radius, def.orbitDist, def.orbitDist);
    }
}

} // namespace scene
