// =============================================================================
// SolarSystem.cpp — All solar system bodies (visualization-scale data)
//
// IMPORTANT: All sizes and distances are *deliberately compressed* for visibility.
// Real astronomical values span ~5 orders of magnitude.
// Every number is chosen so the whole system fits in one viewport and
// every body is clearly distinguishable.
//
// ── Indices ──────────────────────────────────────────────────────────────────
//   m_bodies    : [0]=Sun [1]=Mercury [2]=Venus [3]=Earth [4]=Mars
//                 [5]=Jupiter [6]=Saturn [7]=Uranus [8]=Neptune
//   m_moons     : [0]=Earth-Moon  [1]=Phobos [2]=Deimos
//                 [3]=Io [4]=Europa [5]=Ganymede [6]=Callisto
//                 [7]=Titan [8]=Enceladus [9]=Rhea [10]=Iapetus
//                 [11]=Triton  [12]=Titania [13]=Miranda
//   m_asteroids : [0]=1 Ceres [1]=4 Vesta [2]=2 Pallas
//                 [3]=10 Hygiea [4]=433 Eros [5]=16 Psyche
//   m_comets    : [0]=Halley's Comet  [1]=Long-period comet
// =============================================================================

#include "scene/SolarSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>
#include <cctype>
#include <cmath>
#include <iostream>

namespace scene {

// =============================================================================
// Planet dataset
// =============================================================================
struct BodyDef {
    const char* name;
    glm::vec3   color;
    float       radius;
    float       orbitDist;
    float       orbitSpeed;
    float       rotationSpeed;
};

static const BodyDef k_bodyDefs[] = {
    // name       R    G    B         vis-r  orbit   orb-spd  rot-spd
    { "Sun",     {1.00f,0.85f,0.15f}, 1.65f, 0.0f,   0.0f,    0.2f  },
    { "Mercury", {0.70f,0.68f,0.65f}, 0.15f, 4.0f,   1.5f,    0.3f  },
    { "Venus",   {0.92f,0.80f,0.48f}, 0.22f, 6.0f,   1.0f,   -0.1f  },
    { "Earth",   {0.25f,0.58f,0.95f}, 0.24f, 8.5f,   0.8f,    1.5f  },
    { "Mars",    {0.88f,0.38f,0.14f}, 0.18f,11.0f,   0.6f,    1.4f  },
    { "Jupiter", {0.80f,0.62f,0.42f}, 0.70f,15.0f,   0.4f,    3.0f  },
    { "Saturn",  {0.92f,0.84f,0.58f}, 0.58f,20.0f,   0.25f,   2.8f  },
    { "Uranus",  {0.52f,0.85f,0.92f}, 0.40f,26.0f,   0.15f,  -1.8f  },
    { "Neptune", {0.22f,0.38f,0.92f}, 0.38f,32.0f,   0.10f,   1.7f  },
};

// =============================================================================
// Moon dataset — parent index uses k_bodyDefs ordering:
//   3=Earth  4=Mars  5=Jupiter  6=Saturn  7=Uranus  8=Neptune
// orbitDist is a *multiplier* on the parent's radius so moons scale with planet
// =============================================================================
struct MoonDef {
    const char* name;
    glm::vec3   color;
    float       radius;       // visual radius (world units)
    float       orbitDist;    // local orbit radius in world units (before parent scale)
    float       orbitSpeed;   // rad/s (simulation)
    float       rotationSpeed;
    int         parentIndex;  // index into k_bodyDefs
};

static const MoonDef k_moonDefs[] = {
    // ── Earth ────────────────────────────────────────────────────────────────
    // Index 0  (MUST remain Earth's Moon — legacy API)
    { "Moon",     {0.50f,0.50f,0.50f}, 0.05f, 0.60f,  2.0f,  0.8f,  3 },

    // ── Mars ─────────────────────────────────────────────────────────────────
    { "Phobos",   {0.55f,0.45f,0.35f}, 0.018f, 0.30f, 5.0f,  1.0f,  4 },
    { "Deimos",   {0.50f,0.42f,0.32f}, 0.012f, 0.48f, 3.2f,  0.7f,  4 },

    // ── Jupiter (Galilean moons) ──────────────────────────────────────────────
    { "Io",       {0.95f,0.85f,0.10f}, 0.038f, 0.90f, 4.2f,  1.1f,  5 },  // volcanic yellow
    { "Europa",   {0.85f,0.90f,0.95f}, 0.032f, 1.20f, 3.3f,  0.9f,  5 },  // icy white-blue
    { "Ganymede", {0.55f,0.50f,0.45f}, 0.052f, 1.60f, 2.5f,  0.7f,  5 },  // grey-brown
    { "Callisto", {0.38f,0.35f,0.30f}, 0.048f, 2.10f, 1.9f,  0.6f,  5 },  // dark grey

    // ── Saturn ────────────────────────────────────────────────────────────────
    { "Titan",    {0.80f,0.55f,0.15f}, 0.045f, 1.40f, 2.2f,  0.8f,  6 },  // orange-hazy
    { "Enceladus",{0.90f,0.92f,0.98f}, 0.018f, 0.90f, 4.0f,  1.0f,  6 },  // brilliant white ice
    { "Rhea",     {0.72f,0.70f,0.68f}, 0.028f, 1.15f, 3.1f,  0.9f,  6 },  // grey-white
    { "Iapetus",  {0.60f,0.50f,0.38f}, 0.026f, 1.80f, 1.6f,  0.5f,  6 },  // two-tone brown

    // ── Neptune ────────────────────────────────────────────────────────────────
    { "Triton",   {0.65f,0.78f,0.85f}, 0.035f, 0.80f,-3.5f,  0.8f,  8 },  // retrograde!

    // ── Uranus ────────────────────────────────────────────────────────────────
    { "Titania",  {0.72f,0.72f,0.70f}, 0.028f, 0.90f, 2.8f,  0.7f,  7 },
    { "Miranda",  {0.68f,0.68f,0.65f}, 0.018f, 0.60f, 4.2f,  1.0f,  7 },
};

// =============================================================================
// MoonRecord helpers
// =============================================================================

glm::mat4 MoonRecord::getModelMatrix(const glm::vec3& parentPos, float /*parentRadius*/) const
{
    glm::mat4 parentT = glm::translate(glm::mat4(1.0f), parentPos);
    glm::mat4 orbitT  = glm::translate(glm::mat4(1.0f), body.position);
    glm::mat4 spinS   = glm::rotate(glm::mat4(1.0f),
                                    body.selfRotationAngle,
                                    glm::vec3(0.0f, 1.0f, 0.0f));
    spinS = glm::scale(spinS, glm::vec3(body.radius));

    return parentT * orbitT * spinS;
}

// =============================================================================
// SolarSystem constructor
// =============================================================================

SolarSystem::SolarSystem()
{
    initBodies();
    initMoons();
    initAsteroids();
    initComets();

    // Saturn rings
    m_bodies[6].ringInnerRadius = 1.3f;
    m_bodies[6].ringOuterRadius = 2.5f;
    m_bodies[6].ringTiltAngle   = glm::radians(26.7f);
}

// =============================================================================
// initBodies — planets
// =============================================================================

void SolarSystem::initBodies()
{
    m_bodies.clear();
    m_bodies.reserve(9);

    for (const auto& def : k_bodyDefs)
    {
        CelestialBody body;
        body.name              = def.name;
        body.color             = def.color;
        body.radius            = def.radius;
        body.orbitDistance     = def.orbitDist;
        body.orbitalSpeed      = def.orbitSpeed;
        body.rotationSpeed     = def.rotationSpeed;
        body.currentOrbitAngle = 0.0f;
        body.selfRotationAngle = 0.0f;
        body.position          = glm::vec3(def.orbitDist, 0.0f, 0.0f);
        m_bodies.push_back(body);

        printf("[SolarSystem] %-8s  radius=%.2f  orbit=%.1f  spd=%.2f\n",
               def.name, def.radius, def.orbitDist, def.orbitSpeed);
    }

    // Cache planet textures
    int loaded = 0;
    for (auto& body : m_bodies)
    {
        std::string lower = body.name;
        for (auto& c : lower) c = static_cast<char>(std::tolower(c));
        body.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/" + lower + ".jpg");
        if (!body.texture->hasTexture())
            body.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/" + lower + ".png");
        if (body.texture->hasTexture()) ++loaded;
    }
    std::cout << "[SolarSystem] Planet textures: " << loaded << "/" << m_bodies.size() << "\n";
}

// =============================================================================
// initMoons — all satellites
// =============================================================================

void SolarSystem::initMoons()
{
    m_moons.clear();
    m_moons.reserve(std::size(k_moonDefs));

    int moonLoaded = 0;
    for (const auto& def : k_moonDefs)
    {
        MoonRecord rec;
        rec.parentIndex             = def.parentIndex;
        rec.body.name               = def.name;
        rec.body.color              = def.color;
        rec.body.radius             = def.radius;
        rec.body.orbitDistance      = def.orbitDist;
        rec.body.orbitalSpeed       = def.orbitSpeed;
        rec.body.rotationSpeed      = def.rotationSpeed;
        rec.body.currentOrbitAngle  = 0.0f;
        rec.body.selfRotationAngle  = 0.0f;
        rec.body.position           = glm::vec3(def.orbitDist, 0.0f, 0.0f);

        std::string lower = def.name;
        for (auto& c : lower) c = static_cast<char>(std::tolower(c));
        rec.body.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/" + lower + ".jpg");
        if (!rec.body.texture->hasTexture())
            rec.body.texture = loadTexture(std::string(ASSETS_DIR) + "textures/planets/" + lower + ".png");
        if (rec.body.texture->hasTexture()) ++moonLoaded;

        m_moons.push_back(rec);
    }
    std::cout << "[SolarSystem] Moon textures: " << moonLoaded << "/" << m_moons.size()
              << " (missing ones use body color)\n";
}

// =============================================================================
// initAsteroids — major recognized asteroids & dwarf planets
// =============================================================================

void SolarSystem::initAsteroids()
{
    m_asteroids.clear();

    // 1. Ceres (Dwarf Planet, largest in asteroid belt)
    {
        AsteroidRecord a;
        a.name         = "1 Ceres";
        a.designation  = "Dwarf Planet";
        a.semiMajor    = 12.4f;
        a.inclination  = glm::radians(10.6f);
        a.orbitalSpeed = 0.52f;
        a.currentAngle = 0.35f;

        a.body.name              = "1 Ceres";
        a.body.color             = glm::vec3(0.55f, 0.52f, 0.49f); // clay-carbonate brownish grey with bright salt faculae
        a.body.radius            = 0.085f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 1.1f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/ceres.jpg");

        a.facts[0] = "Largest asteroid belt body; spherical under self-gravity; water ice cryovolcanism.";
        a.facts[1] = "Contains ~30% of belt's mass; hosts Ahuna Mons ice volcano & deep brine reservoirs.";
        a.facts[2] = "NASA's Dawn discovered bright sodium carbonate salt faculae in Occator Crater.";

        m_asteroids.push_back(a);
    }

    // 2. 4 Vesta (Brightest asteroid, protoplanet with massive south pole crater)
    {
        AsteroidRecord a;
        a.name         = "4 Vesta";
        a.designation  = "Differentiated Protoplanet";
        a.semiMajor    = 12.85f;
        a.inclination  = glm::radians(7.1f);
        a.orbitalSpeed = 0.50f;
        a.currentAngle = 1.45f;

        a.body.name              = "4 Vesta";
        a.body.color             = glm::vec3(0.88f, 0.83f, 0.72f); // bright basaltic buff
        a.body.radius            = 0.070f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 1.4f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/vesta.jpg");

        a.facts[0] = "Brightest asteroid visible from Earth; hosts the colossal 500 km Rheasilvia impact basin.";
        a.facts[1] = "Rheasilvia's central peak rises 22 km high — over twice the height of Mount Everest!";
        a.facts[2] = "Differentiated protoplanet with metallic core; source of ~5% of Earth's meteorites.";

        m_asteroids.push_back(a);
    }

    // 3. 2 Pallas (Steep orbital inclination of 34.8 degrees)
    {
        AsteroidRecord a;
        a.name         = "2 Pallas";
        a.designation  = "B-type Asteroid";
        a.semiMajor    = 13.30f;
        a.inclination  = glm::radians(34.8f); // famous steep inclination
        a.orbitalSpeed = 0.48f;
        a.currentAngle = 2.60f;

        a.body.name              = "2 Pallas";
        a.body.color             = glm::vec3(0.52f, 0.57f, 0.65f); // B-type slate bluish-grey
        a.body.radius            = 0.065f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 1.2f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/pallas.jpg");

        a.facts[0] = "High orbital inclination of 34.8° to the ecliptic plane; battered golf-ball surface.";
        a.facts[1] = "Violent collision velocity creates a heavily cratered 'golf-ball' surface.";
        a.facts[2] = "Discovered by Heinrich Olbers in 1802; chemical element Palladium is named after it.";

        m_asteroids.push_back(a);
    }

    // 4. 10 Hygiea (Fourth largest, spherical dark carbonaceous)
    {
        AsteroidRecord a;
        a.name         = "10 Hygiea";
        a.designation  = "C-type Dwarf Planet Candidate";
        a.semiMajor    = 13.75f;
        a.inclination  = glm::radians(3.8f);
        a.orbitalSpeed = 0.45f;
        a.currentAngle = 3.90f;

        a.body.name              = "10 Hygiea";
        a.body.color             = glm::vec3(0.24f, 0.23f, 0.22f); // extremely dark primitive carbonaceous soot
        a.body.radius            = 0.060f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 0.9f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/hygiea.jpg");

        a.facts[0] = "Extremely dark carbonaceous primitive surface; remarkably spherical.";
        a.facts[1] = "VLT SPHERE imaging revealed it is spherical under self-gravity (dwarf planet candidate).";
        a.facts[2] = "Extremely low albedo (~0.07) carbonaceous surface, nearly pristine primitive material.";

        m_asteroids.push_back(a);
    }

    // 5. 433 Eros (Near-Earth asteroid, visited by NEAR Shoemaker)
    {
        AsteroidRecord a;
        a.name         = "433 Eros";
        a.designation  = "Near-Earth Asteroid";
        a.semiMajor    = 10.40f; // close to Mars (11.0f)
        a.inclination  = glm::radians(10.8f);
        a.orbitalSpeed = 0.65f;
        a.currentAngle = 4.80f;

        a.body.name              = "433 Eros";
        a.body.color             = glm::vec3(0.78f, 0.65f, 0.50f); // silicate warm golden-tan
        a.body.radius            = 0.045f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 1.8f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/eros.jpg");

        a.facts[0] = "Peanut-shaped Mars-crosser; first asteroid orbited and landed on by a spacecraft (NEAR Shoemaker).";
        a.facts[1] = "Elongated peanut geometry (34x11x11 km) with drastically varying surface gravity.";
        a.facts[2] = "Silicate S-type asteroid with smooth ponded regolith deposits in depressions.";

        m_asteroids.push_back(a);
    }

    // 6. 16 Psyche (Unique metallic nickel-iron asteroid)
    {
        AsteroidRecord a;
        a.name         = "16 Psyche";
        a.designation  = "M-type Metallic Asteroid";
        a.semiMajor    = 14.20f;
        a.inclination  = glm::radians(3.1f);
        a.orbitalSpeed = 0.43f;
        a.currentAngle = 5.70f;

        a.body.name              = "16 Psyche";
        a.body.color             = glm::vec3(0.78f, 0.78f, 0.82f); // metallic nickel-iron sheen
        a.body.radius            = 0.055f;
        a.body.orbitDistance     = a.semiMajor;
        a.body.orbitalSpeed      = a.orbitalSpeed;
        a.body.rotationSpeed     = 1.5f;
        a.body.currentOrbitAngle = a.currentAngle;
        a.body.selfRotationAngle = 0.0f;
        a.body.position          = glm::vec3(a.semiMajor, 0.0f, 0.0f);
        a.body.texture           = loadTexture(std::string(ASSETS_DIR) + "textures/planets/psyche.jpg");

        a.facts[0] = "Exposed metallic nickel-iron protoplanetary core; NASA Psyche mission target.";
        a.facts[1] = "Believed to be the exposed core of a protoplanet stripped by violent ancient collisions.";
        a.facts[2] = "M-type metallic asteroid with high radar reflectivity and Widmanstätten patterns.";

        m_asteroids.push_back(a);
    }

    std::cout << "[SolarSystem] Initialized " << m_asteroids.size() << " major named asteroids (Ceres, Vesta, Pallas, Hygiea, Eros, Psyche) with distinct textures & geometry." << std::endl;
}

// =============================================================================
// initComets
// =============================================================================

void SolarSystem::initComets()
{
    m_comets.clear();

    // ── Halley's Comet ────────────────────────────────────────────────────────
    {
        CometRecord c;
        c.name        = "Halley's Comet";
        c.description = "Famous short-period comet. Real period ~75 yr. "
                        "Highly inclined retrograde orbit (e~0.97).";
        c.semiMajor   = 22.0f;     // compressed
        c.eccentricity= 0.85f;
        c.inclination = glm::radians(162.0f); // retrograde, high inclination
        c.orbitalSpeed= 0.08f;
        c.currentAngle= 0.0f;

        c.body.name          = "Halley's Comet";
        c.body.color         = glm::vec3(0.85f, 0.92f, 1.0f); // icy blue-white
        c.body.radius        = 0.06f;
        c.body.orbitDistance = c.semiMajor;
        c.body.orbitalSpeed  = c.orbitalSpeed;
        c.body.rotationSpeed = 0.3f;
        c.body.currentOrbitAngle = 0.0f;
        c.body.selfRotationAngle = 0.0f;
        c.body.position      = glm::vec3(c.semiMajor, 0.0f, 0.0f);
        c.tailDir            = glm::vec3(1.0f, 0.0f, 0.0f);
        c.distFromSun        = c.semiMajor;
        c.body.texture       = nullptr; // procedural
        m_comets.push_back(c);
    }

    // ── Long-period Comet (Hale-Bopp approx.) ──────────────────────────────────
    {
        CometRecord c;
        c.name        = "Hale-Bopp (approx.)";
        c.description = "Approximation of a long-period comet (e~0.97). "
                        "Simplified ellipse; not physically complete.";
        c.semiMajor   = 28.0f;
        c.eccentricity= 0.92f;
        c.inclination = glm::radians(89.4f);
        c.orbitalSpeed= 0.05f;
        c.currentAngle= glm::pi<float>() * 0.6f;

        c.body.name          = "Hale-Bopp (approx.)";
        c.body.color         = glm::vec3(0.95f, 0.95f, 0.80f); // warm white
        c.body.radius        = 0.05f;
        c.body.orbitDistance = c.semiMajor;
        c.body.orbitalSpeed  = c.orbitalSpeed;
        c.body.rotationSpeed = 0.2f;
        c.body.currentOrbitAngle = c.currentAngle;
        c.body.selfRotationAngle = 0.0f;
        c.body.position      = glm::vec3(c.semiMajor, 0.0f, 0.0f);
        c.tailDir            = glm::vec3(1.0f, 0.0f, 0.0f);
        c.distFromSun        = c.semiMajor;
        c.body.texture       = nullptr;
        m_comets.push_back(c);
    }
}

// =============================================================================
// update — advance all bodies
// =============================================================================

void SolarSystem::update(float deltaTime)
{
    // ── Planets ──────────────────────────────────────────────────────────────
    for (auto& body : m_bodies)
    {
        body.currentOrbitAngle += body.orbitalSpeed  * deltaTime;
        body.selfRotationAngle += body.rotationSpeed * deltaTime;
        body.position.x = std::cos(body.currentOrbitAngle) * body.orbitDistance;
        body.position.z = std::sin(body.currentOrbitAngle) * body.orbitDistance;
    }

    // ── Moons ─────────────────────────────────────────────────────────────────
    for (auto& rec : m_moons)
    {
        auto& b = rec.body;
        b.currentOrbitAngle += b.orbitalSpeed  * deltaTime;
        b.selfRotationAngle += b.rotationSpeed * deltaTime;
        b.position.x = std::cos(b.currentOrbitAngle) * b.orbitDistance;
        b.position.z = std::sin(b.currentOrbitAngle) * b.orbitDistance;
        b.position.y = 0.0f;
    }

    // ── Asteroids (with orbital inclination in 3D) ───────────────────────────
    for (auto& ast : m_asteroids)
    {
        ast.currentAngle += ast.orbitalSpeed * deltaTime;
        const float r = ast.semiMajor;
        const float x0 = r * std::cos(ast.currentAngle);
        const float z0 = r * std::sin(ast.currentAngle);
        const float ci = std::cos(ast.inclination);
        const float si = std::sin(ast.inclination);

        ast.body.position.x = x0;
        ast.body.position.y = z0 * si;
        ast.body.position.z = z0 * ci;
        ast.body.currentOrbitAngle = ast.currentAngle;
        ast.body.selfRotationAngle += ast.body.rotationSpeed * deltaTime;
    }

    // ── Comets (simplified elliptical orbit in 3-D) ───────────────────────────
    for (auto& c : m_comets)
    {
        c.currentAngle += c.orbitalSpeed * deltaTime;

        const float a = c.semiMajor;
        const float e = c.eccentricity;
        const float r = a * (1.0f - e * e) / (1.0f + e * std::cos(c.currentAngle));

        const float x0 = r * std::cos(c.currentAngle);
        const float z0 = r * std::sin(c.currentAngle);

        const float ci = std::cos(c.inclination);
        const float si = std::sin(c.inclination);

        c.body.position.x = x0;
        c.body.position.y = z0 * si;
        c.body.position.z = z0 * ci;

        c.body.selfRotationAngle += c.body.rotationSpeed * deltaTime;
        c.body.currentOrbitAngle  = c.currentAngle;

        glm::vec3 fromSun = c.body.position;
        c.distFromSun = glm::length(fromSun);
        if (c.distFromSun > 0.01f)
            c.tailDir = glm::normalize(fromSun);
        else
            c.tailDir = glm::vec3(1.0f, 0.0f, 0.0f);
    }
}

// =============================================================================
// Tour Stop Sequence (Planets + Asteroids in outward orbital order)
//
//   0: Sun
//   1: Mercury
//   2: Venus
//   3: Earth
//   4: Mars
//   5: 1 Ceres (Dwarf Planet)
//   6: 4 Vesta
//   7: 2 Pallas
//   8: 10 Hygiea
//   9: 433 Eros
//  10: 16 Psyche
//  11: Jupiter
//  12: Saturn
//  13: Uranus
//  14: Neptune
// =============================================================================

static const char* k_planetFacts[9][3] = {
    // Sun (0)
    {"The Sun is the only emissive light source in the simulation.",
     "It uses a special unlit shader to bypass Blinn-Phong shading.",
     "A volumetric glow billboard surrounds it."},
    // Mercury (1)
    {"Fastest orbital speed in the system.",
     "Smallest planet, closest to the Sun.",
     "No atmosphere to retain heat."},
    // Venus (2)
    {"Rotates backwards compared to most planets.",
     "Visually distinct due to its dense cloud cover.",
     "Second planet from the Sun."},
    // Earth (3)
    {"Features a custom procedural Fresnel atmosphere.",
     "Home to a hierarchically orbiting Moon.",
     "The only known planet with abundant liquid water."},
    // Mars (4)
    {"The Red Planet, coloured by surface iron oxide.",
     "Roughly half the size of Earth in this simulator.",
     "Has two tiny moons: Phobos and Deimos."},
    // Jupiter (5)
    {"The largest planet, dwarfing all others combined.",
     "Could fit over 1,000 Earths inside its volume.",
     "Famous for its Great Red Spot and 4 Galilean moons."},
    // Saturn (6)
    {"Features procedural annulus rings built from a triangle strip.",
     "Rings are tilted at a fixed 26.7 degrees independent of spin.",
     "The second largest planet, surrounded by Titan and Enceladus."},
    // Uranus (7)
    {"An ice giant with a distinctive pale cyan hue.",
     "Rotates entirely on its side at an axis tilt of 97.8 degrees.",
     "Seventh planet from the Sun."},
    // Neptune (8)
    {"The outermost planet, orbiting the slowest.",
     "Its wide orbit requires a far camera clipping plane over 80.0 units.",
     "Known for supersonic wind speeds and retrograde moon Triton."}
};

int SolarSystem::getTourStopCount() const
{
    // Sun (1) + 4 inner planets + m_asteroids + 4 outer planets
    return 1 + 4 + static_cast<int>(m_asteroids.size()) + 4;
}

TourStopInfo SolarSystem::getTourStop(int index) const
{
    TourStopInfo info;
    info.facts[0] = "";
    info.facts[1] = "";
    info.facts[2] = "";

    // 0: Sun
    if (index == 0)
    {
        info.name     = m_bodies[0].name;
        info.position = m_bodies[0].position;
        info.radius   = m_bodies[0].radius;
        info.facts[0] = k_planetFacts[0][0];
        info.facts[1] = k_planetFacts[0][1];
        info.facts[2] = k_planetFacts[0][2];
        return info;
    }

    // 1–4: Inner terrestrial planets (Mercury, Venus, Earth, Mars)
    if (index >= 1 && index <= 4)
    {
        info.name     = m_bodies[index].name;
        info.position = m_bodies[index].position;
        info.radius   = m_bodies[index].radius;
        info.facts[0] = k_planetFacts[index][0];
        info.facts[1] = k_planetFacts[index][1];
        info.facts[2] = k_planetFacts[index][2];
        return info;
    }

    // 5–10: Major Asteroids (Ceres, Vesta, Pallas, Hygiea, Eros, Psyche)
    const int astCount = static_cast<int>(m_asteroids.size());
    if (index >= 5 && index < 5 + astCount)
    {
        int astIdx = index - 5;
        const auto& ast = m_asteroids[astIdx];
        info.name     = ast.name + " [" + ast.designation + "]";
        info.position = ast.body.position;
        info.radius   = ast.body.radius;
        info.facts[0] = ast.facts[0].c_str();
        info.facts[1] = ast.facts[1].c_str();
        info.facts[2] = ast.facts[2].c_str();
        return info;
    }

    // 11–14: Outer gas & ice giants (Jupiter, Saturn, Uranus, Neptune)
    int outerIdx = index - astCount; // when index=11, outerIdx=5 (Jupiter)
    if (outerIdx >= 5 && outerIdx < static_cast<int>(m_bodies.size()))
    {
        info.name     = m_bodies[outerIdx].name;
        info.position = m_bodies[outerIdx].position;
        info.radius   = m_bodies[outerIdx].radius;
        info.facts[0] = k_planetFacts[outerIdx][0];
        info.facts[1] = k_planetFacts[outerIdx][1];
        info.facts[2] = k_planetFacts[outerIdx][2];
        return info;
    }

    // Fallback safety
    info.name     = "Celestial Object";
    info.position = glm::vec3(0.0f);
    info.radius   = 1.0f;
    return info;
}

// =============================================================================
// Texture cache helper
// =============================================================================

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

// =============================================================================
// Legacy Earth–Moon accessor (index 0 in m_moons)
// =============================================================================

glm::mat4 SolarSystem::getMoonModelMatrix() const
{
    const CelestialBody& earth = m_bodies[3];
    const CelestialBody& moon  = m_moons[0].body;

    glm::mat4 earthWorld = earth.getModelMatrix();
    glm::mat4 moonOrbit  = glm::translate(glm::mat4(1.0f), moon.position);
    glm::mat4 moonSpinS  = glm::rotate(glm::mat4(1.0f),
                                       moon.selfRotationAngle,
                                       glm::vec3(0.0f, 1.0f, 0.0f));
    moonSpinS = glm::scale(moonSpinS, glm::vec3(moon.radius));

    return earthWorld * moonOrbit * moonSpinS;
}

} // namespace scene
