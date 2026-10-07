#pragma once
// =============================================================================
// Camera.h — Orbit-style camera for a solar system viewer
//
// CG concept — View transformation:
//   The camera does NOT move in OpenGL; instead we transform the entire world
//   so it appears as if we moved. glm::lookAt() builds this "view matrix".
//
// CG concept — Perspective projection:
//   Objects further away appear smaller. glm::perspective() encodes this by
//   mapping the view frustum (a truncated pyramid) into the clip cube [-1,1]³.
//   The result is the "projection matrix".
//
// Orbit camera model:
//   The camera always looks at a fixed target point. Its position is defined
//   in spherical coordinates — distance r, yaw θ (horizontal angle), and
//   pitch φ (vertical angle) — which are converted to Cartesian for lookAt.
// =============================================================================

#include <glm/glm.hpp>

namespace camera {

class Camera
{
public:
    // ── Construction ──────────────────────────────────────────────────────────
    /// @param target    World-space point the camera orbits around
    /// @param distance  How far from the target the camera sits
    /// @param yaw       Horizontal angle in radians (0 = +Z, π/2 = +X)
    /// @param pitch     Vertical angle in radians (0 = equator, clamped ±89°)
    /// @param fovDeg    Vertical field of view in degrees
    Camera(glm::vec3 target   = glm::vec3(0.0f),
           float     distance = 5.0f,
           float     yaw      = 0.0f,
           float     pitch    = 0.3f,
           float     fovDeg   = 45.0f);

    // ── Matrix producers ──────────────────────────────────────────────────────
    /// Returns the view matrix (world → camera space)
    glm::mat4 getViewMatrix()      const;

    /// Returns the projection matrix (camera → clip space)
    /// @param aspect  Viewport width / height
    glm::mat4 getProjectionMatrix(float aspect,
                                  float nearPlane = 0.1f,
                                  float farPlane  = 1000.0f) const;

    // ── Position query ────────────────────────────────────────────────────────
    /// Computes and returns the camera eye position in world space
    glm::vec3 getPosition() const;

    // ── Orbit controls (call from input handlers in later phases) ─────────────
    void setTarget  (const glm::vec3& t) { m_target   = t;                        }
    void setDistance(float d)            { m_distance = glm::max(d, 0.01f);       }
    void setYaw     (float y)            { m_yaw      = y;                        }
    void setPitch   (float p)            { m_pitch    = glm::clamp(p, -1.553f, 1.553f); } // ±89°
    void setFov     (float f)            { m_fov      = glm::clamp(f, 10.0f, 120.0f);   }

    void adjustDistance(float delta) { setDistance(m_distance + delta); }
    void adjustYaw     (float delta) { m_yaw   += delta;                }
    void adjustPitch   (float delta) { setPitch(m_pitch + delta);       }

    // ── Getters ───────────────────────────────────────────────────────────────
    float     getDistance() const { return m_distance; }
    float     getYaw()      const { return m_yaw;      }
    float     getPitch()    const { return m_pitch;    }
    float     getFov()      const { return m_fov;      }
    glm::vec3 getTarget()   const { return m_target;   }

private:
    glm::vec3 m_target;
    float     m_distance;
    float     m_yaw;
    float     m_pitch;
    float     m_fov;
};

} // namespace camera
