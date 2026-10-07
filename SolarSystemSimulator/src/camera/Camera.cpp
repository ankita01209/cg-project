// =============================================================================
// Camera.cpp — Orbit camera implementation
// =============================================================================

#include "camera/Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace camera {

Camera::Camera(glm::vec3 target, float distance, float yaw, float pitch, float fovDeg)
    : m_target(target)
    , m_distance(distance)
    , m_yaw(yaw)
    , m_pitch(pitch)
    , m_fov(fovDeg)
{}

// ── Position from spherical coordinates ───────────────────────────────────────
// CG concept — Spherical → Cartesian:
//   Any point on a sphere of radius r can be described by two angles.
//   Yaw (θ) rotates around the Y-axis (left/right).
//   Pitch (φ) elevates above or below the equatorial plane (up/down).
//
//   eye.x = target.x + r · cos(φ) · sin(θ)
//   eye.y = target.y + r · sin(φ)
//   eye.z = target.z + r · cos(φ) · cos(θ)
glm::vec3 Camera::getPosition() const
{
    float cosP = std::cos(m_pitch);
    float sinP = std::sin(m_pitch);
    float cosY = std::cos(m_yaw);
    float sinY = std::sin(m_yaw);

    return m_target + glm::vec3(
        m_distance * cosP * sinY,   // x
        m_distance * sinP,          // y
        m_distance * cosP * cosY    // z
    );
}

// ── View matrix ───────────────────────────────────────────────────────────────
// CG concept — View / Camera transform:
//   Translates and rotates the world so that the camera sits at the origin
//   looking down -Z. glm::lookAt builds this matrix from:
//     eye    : camera position in world space
//     center : the point the camera aims at
//     up     : which direction is "up" (world Y axis, unless pitch is ±90°)
glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(getPosition(), m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

// ── Projection matrix ─────────────────────────────────────────────────────────
// CG concept — Perspective projection:
//   The frustum (truncated pyramid between nearPlane and farPlane) is mapped
//   to the clip cube [-1,1]³ by dividing by the homogeneous w component.
//   This makes far objects smaller — the essential "depth cue" of 3D graphics.
//
//   fovY      : vertical angle of the view cone
//   aspect    : width/height ratio (prevents squashing on non-square windows)
//   nearPlane : fragments closer than this are clipped (avoid ≤ 0)
//   farPlane  : fragments further than this are clipped
glm::mat4 Camera::getProjectionMatrix(float aspect, float nearPlane, float farPlane) const
{
    return glm::perspective(glm::radians(m_fov), aspect, nearPlane, farPlane);
}

} // namespace camera
