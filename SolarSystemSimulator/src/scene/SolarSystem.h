#pragma once
// =============================================================================
// SolarSystem.h â€” Creates and owns all solar system bodies
//
// Responsibilities:
//   â€¢ Holds the master list of CelestialBody records (Sun + 8 planets).
//   â€¢ On construction populates every body's name, colour, size and position.
//   â€¢ Exposes a const accessor so the render loop can iterate without mutation.
// =============================================================================

#include "scene/CelestialBody.h"
#include "render/Texture.h"
#include <vector>
#include <memory>
#include <unordered_map>

namespace scene {

class SolarSystem
{
public:
    /// Initialises all 9 bodies using compressed visualization-scale data.
    SolarSystem();

    /// Advances the simulation by deltaTime seconds.
    void update(float deltaTime);

    const std::vector<CelestialBody>& getBodies() const { return m_bodies; }

    // Texture UI Toggles
    bool isTexturesEnabled() const { return m_texturesEnabled; }
    void setTexturesEnabled(bool enabled) const { m_texturesEnabled = enabled; }

    // â”€â”€ Phase 7: Hierarchy (Moon) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    const CelestialBody& getMoon() const { return m_moon; }
    glm::mat4 getMoonModelMatrix() const;

private:
    std::vector<CelestialBody> m_bodies;
    CelestialBody              m_moon;

    std::unordered_map<std::string, std::unique_ptr<render::Texture>> m_textureCache;
    render::Texture* loadTexture(const std::string& path);
    mutable bool m_texturesEnabled = true;

    void initBodies();
};

} // namespace scene
