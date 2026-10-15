#pragma once
// =============================================================================
// SolarSystem.h — Creates and owns all solar system bodies
//
// Responsibilities:
//   • Holds the master list of CelestialBody records (Sun + 8 planets).
//   • Holds all additional moons as MoonRecord (parent-child hierarchy).
//   • Holds comet bodies that follow elliptical-ish orbits.
//   • Holds major named asteroids (Ceres, Vesta, Pallas, Hygiea, Eros, Psyche).
//   • Provides tour stops sequence covering planets & asteroids with fun facts.
//   • Exposes const accessors so the render loop can iterate without mutation.
// =============================================================================

#include "scene/CelestialBody.h"
#include "render/Texture.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

namespace scene {

// ---------------------------------------------------------------------------
// MoonRecord: one satellite attached to a parent body (by index into m_bodies)
// ---------------------------------------------------------------------------
struct MoonRecord
{
    CelestialBody body;
    int           parentIndex; ///< index into SolarSystem::m_bodies (e.g. 3=Earth, 4=Mars...)

    /// Compute world-space model matrix using parent's current position.
    glm::mat4 getModelMatrix(const glm::vec3& parentPos, float parentRadius) const;
};

// ---------------------------------------------------------------------------
// CometRecord: a comet with an elongated orbit and a tail direction
// ---------------------------------------------------------------------------
struct CometRecord
{
    CelestialBody body;         ///< nucleus size/color/position
    float         semiMajor;    ///< orbit semi-major axis (world units)
    float         eccentricity; ///< orbit eccentricity (0=circle, <1 ellipse)
    float         inclination;  ///< orbit plane inclination (radians)
    float         orbitalSpeed; ///< mean angular speed (rad/s, simulation-scale)
    float         currentAngle; ///< current mean anomaly (radians)
    std::string   name;
    std::string   description;
    glm::vec3     tailDir;      ///< cached direction away from Sun (updated each frame)
    float         distFromSun;  ///< cached scalar, used for tail length scaling
};

// ---------------------------------------------------------------------------
// AsteroidRecord: an individual recognized asteroid with orbital parameters
// ---------------------------------------------------------------------------
struct AsteroidRecord
{
    CelestialBody body;
    float         semiMajor;    ///< orbital radius (semi-major axis)
    float         inclination;  ///< orbital inclination in radians
    float         orbitalSpeed; ///< rad/s in simulation
    float         currentAngle; ///< current anomaly in radians
    std::string   name;
    std::string   designation;  ///< e.g. "Dwarf Planet", "V-type Asteroid"
    std::string   facts[3];     ///< 3 educational fun facts
};

// ---------------------------------------------------------------------------
// TourStopInfo: universal information for a cinematic tour stop
// ---------------------------------------------------------------------------
struct TourStopInfo
{
    std::string name;
    glm::vec3   position;
    float       radius;
    const char* facts[3];
};

// ---------------------------------------------------------------------------
class SolarSystem
{
public:
    /// Initialises all bodies (planets, moons, comets, asteroids).
    SolarSystem();

    /// Advances the simulation by deltaTime seconds (scaled).
    void update(float deltaTime);

    const std::vector<CelestialBody>&  getBodies()    const { return m_bodies; }
    const std::vector<MoonRecord>&     getMoons()     const { return m_moons;  }
    const std::vector<CometRecord>&    getComets()    const { return m_comets; }
    const std::vector<AsteroidRecord>& getAsteroids() const { return m_asteroids; }

    // Texture UI Toggles
    bool isTexturesEnabled() const { return m_texturesEnabled; }
    void setTexturesEnabled(bool enabled) const { m_texturesEnabled = enabled; }

    // ── Tour Stop Queries (Planets + Asteroids in physical outward sequence) ──
    int          getTourStopCount() const;
    TourStopInfo getTourStop(int index) const;

    // ── Legacy (Earth–Moon) accessors — kept for backward compat ──────────────
    const CelestialBody& getMoon()          const { return m_moons[0].body; }
    glm::mat4            getMoonModelMatrix() const;

    // ── Mutable access for Comets and Asteroids ──────────────────────────────
    std::vector<CometRecord>&    getCometsMut()    { return m_comets; }
    std::vector<AsteroidRecord>& getAsteroidsMut() { return m_asteroids; }

private:
    std::vector<CelestialBody>  m_bodies;    ///< Sun (0) + 8 planets (1–8)
    std::vector<MoonRecord>     m_moons;     ///< All moons (index 0 = Earth's Moon)
    std::vector<CometRecord>    m_comets;
    std::vector<AsteroidRecord> m_asteroids; ///< Major named asteroids

    std::unordered_map<std::string, std::unique_ptr<render::Texture>> m_textureCache;
    render::Texture* loadTexture(const std::string& path);
    mutable bool m_texturesEnabled = true;

    void initBodies();
    void initMoons();
    void initComets();
    void initAsteroids();
};

} // namespace scene
