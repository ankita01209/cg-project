#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace scene {

// Keeps track of physical layout of a single procedural asteroid
struct Asteroid
{
    glm::vec3 position;
    glm::vec3 color;

    float orbitDistance;
    float currentOrbitAngle;
    float orbitalSpeed;

    glm::vec3 rotationAxis;
    float selfRotationAngle;
    float rotationSpeed;

    float scale;
    float verticalOffset;

    glm::mat4 getModelMatrix() const
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = glm::translate(model, glm::vec3(0.0f, verticalOffset, 0.0f));
        model = glm::rotate(model, selfRotationAngle, rotationAxis);
        model = glm::scale(model, glm::vec3(scale));
        return model;
    }
};

} // namespace scene
