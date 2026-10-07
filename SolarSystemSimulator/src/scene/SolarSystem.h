#pragma once
// =============================================================================
// SolarSystem.h — Creates and owns all solar system bodies
//
// Responsibilities:
//   • Holds the master list of CelestialBody records (Sun + 8 planets).
//   • On construction populates every body's name, colour, size and position.
//   • Exposes a const accessor so the render loop can iterate without mutation.
// =============================================================================

#include "scene/CelestialBody.h"
#include <vector>

namespace scene {

class SolarSystem
{
public:
    /// Initialises all 9 bodies using compressed visualization-scale data.
    SolarSystem();

    const std::vector<CelestialBody>& getBodies() const { return m_bodies; }

private:
    std::vector<CelestialBody> m_bodies;

    void initBodies();
};

} // namespace scene
