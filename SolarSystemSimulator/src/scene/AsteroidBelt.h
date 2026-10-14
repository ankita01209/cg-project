#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "scene/Asteroid.h"
#include "render/Mesh.h"
#include "render/RenderStats.h"
#include "core/Shader.h"

namespace scene {

class AsteroidBelt
{
public:
    AsteroidBelt();
    ~AsteroidBelt();

    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    void setCount(int count);
    int  getCount() const { return m_count; }

    void update(float deltaTime);
    // Feature 1: pass stats pointer to record instanced draw calls (nullptr = disabled)
    void draw(core::Shader& shader, render::RenderStats* stats = nullptr);

private:
    void generateAsteroids();
    void buildMeshes();
    void initInstancing();
    void updateInstanceBuffer();

    bool m_enabled = false;
    int  m_count = 250;

    std::vector<Asteroid> m_asteroids;

    // Meshes and GPU Instancing
    // We use a small collection of irregular sphere-based meshes to look rocky
    std::vector<render::Mesh*> m_meshes;
    GLuint m_instanceVBO = 0;
};

} // namespace scene
