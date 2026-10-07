// =============================================================================
// CelestialBody.cpp
// =============================================================================

#include "scene/CelestialBody.h"
#include <glm/gtc/matrix_transform.hpp>

namespace scene {

// =============================================================================
// getModelMatrix
//
// CG concept — Transform Composition (T * R * S):
//   Matrix multiplication is NOT commutative (A * B != B * A).
//   When building a model matrix, we read right-to-left in code, but conceptually
//   the operations apply to the object in this order: Scale, then Rotate, then Translate.
//
//   1. Scale (S): Resizes the unit-sphere to the planet's radius. Happens first
//      while the sphere is centred at the origin.
//   2. Rotate (R): Spins the planet around its own axis (the Y axis). If we translated
//      first, this rotation would swing the planet in a massive arc around the origin!
//   3. Translate (T): Moves the rotated, scaled planet to its final orbit position.
//
//   Code order: model = Translate * Rotate * Scale
// =============================================================================
glm::mat4 CelestialBody::getModelMatrix() const
{
    glm::mat4 m = glm::mat4(1.0f);

    // 3. Translation (applied last)
    m = glm::translate(m, position);

    // 2. Rotation (applied second)
    m = glm::rotate(m, selfRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. Scale (applied first)
    m = glm::scale(m, glm::vec3(radius));

    return m;
}

} // namespace scene
