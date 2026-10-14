#include "scene/AsteroidBelt.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>

namespace scene {

// Helper to generate a perturbed sphere mesh directly bypassing Sphere.cpp linking dependencies
static render::Mesh createAsteroidMesh(int seed, float radius, int stacks, int slices)
{
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> distOffset(-0.2f, 0.2f);

    std::vector<render::Vertex>       vertices;
    std::vector<unsigned int>         indices;
    vertices.reserve((stacks + 1) * (slices + 1));
    const float pi = glm::pi<float>();
    const float twoPi = glm::two_pi<float>();
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = static_cast<float>(i) * pi / static_cast<float>(stacks);
        float sinP = std::sin(phi);
        float cosP = std::cos(phi);
        for (int j = 0; j <= slices; ++j)
        {
            float theta = static_cast<float>(j) * twoPi / static_cast<float>(slices);
            float sinT  = std::sin(theta);
            float cosT  = std::cos(theta);
            glm::vec3 unitPos(sinP * cosT, cosP, sinP * sinT);

            float displacement = 1.0f + distOffset(rng);

            render::Vertex v;
            v.position = unitPos * radius * displacement;

            // Recalculating strict precise normals for randomly displaced vertices is complex;
            // We approximate it with the unit surface normal for low-poly stylized look.
            v.normal   = unitPos;
            v.texCoord = glm::vec2(static_cast<float>(j) / slices, static_cast<float>(i) / stacks);
            vertices.push_back(v);
        }
    }

    indices.reserve(stacks * slices * 6);
    int rowLen = slices + 1;
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            unsigned int tl = i * rowLen + j;
            unsigned int tr = i * rowLen + j + 1;
            unsigned int bl = (i + 1) * rowLen + j;
            unsigned int br = (i + 1) * rowLen + j + 1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(br);
            indices.push_back(tl); indices.push_back(br); indices.push_back(tr);
        }
    }
    return render::Mesh(vertices, indices);
}

AsteroidBelt::AsteroidBelt()
{
    buildMeshes();
    generateAsteroids();
    initInstancing();
}

AsteroidBelt::~AsteroidBelt()
{
    for (auto* m : m_meshes) delete m;
    if (m_instanceVBO != 0) glDeleteBuffers(1, &m_instanceVBO);
}

void AsteroidBelt::setCount(int count)
{
    if (m_count != count)
    {
        m_count = count;
        generateAsteroids();
    }
}

void AsteroidBelt::buildMeshes()
{
    // Generate 3 irregular asteroid shapes
    m_meshes.push_back(new render::Mesh(createAsteroidMesh(1337, 1.0f, 12, 12)));
    m_meshes.push_back(new render::Mesh(createAsteroidMesh(4242, 1.0f, 10, 10)));
    m_meshes.push_back(new render::Mesh(createAsteroidMesh(9999, 1.0f, 8, 8)));
}

void AsteroidBelt::initInstancing()
{
    glGenBuffers(1, &m_instanceVBO);
    // Buffer struct: mat4 (16 floats) + vec3 color (3 floats) + padding (1) = 20 floats (80 bytes) per instance
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, 400 * sizeof(glm::mat4) + 400 * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);

    // We bind the attributes per mesh
    for (auto* m : m_meshes)
    {
        glBindVertexArray(m->getVAO());
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

        GLsizei vec4Size = sizeof(glm::vec4);
        GLsizei instanceStride = 4 * vec4Size + vec4Size; // 4 rows + 1 color padded vec4

        // modelMatrix (Location 3..6)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, instanceStride, (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, instanceStride, (void*)(1 * vec4Size));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, instanceStride, (void*)(2 * vec4Size));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, instanceStride, (void*)(3 * vec4Size));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        // color (Location 7)
        glEnableVertexAttribArray(7);
        glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, instanceStride, (void*)(4 * vec4Size));
        glVertexAttribDivisor(7, 1);

        glBindVertexArray(0);
    }
}

void AsteroidBelt::generateAsteroids()
{
    m_asteroids.clear();
    m_asteroids.reserve(m_count);

    std::mt19937 rng(42);

    float marsR = 11.0f;
    float jupR = 15.0f;
    float gap = jupR - marsR;
    float innerR = marsR + gap * 0.25f; // approx 12.0
    float outerR = jupR - gap * 0.25f;  // approx 14.0

    std::uniform_real_distribution<float> dRadius(innerR, outerR);
    std::uniform_real_distribution<float> dAngle(0.0f, glm::two_pi<float>());
    std::uniform_real_distribution<float> dScale(0.015f, 0.05f);
    std::uniform_real_distribution<float> dVert(-0.3f, 0.3f);
    std::uniform_real_distribution<float> dRotSpd(0.5f, 2.0f);
    std::uniform_real_distribution<float> dAxis(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dColorVar(-0.1f, 0.1f);

    for (int i = 0; i < m_count; ++i)
    {
        Asteroid a;
        a.orbitDistance = dRadius(rng);

        // Slight clustering trick: average of two uniforms makes a triangle distribution
        if (i % 2 == 0) a.orbitDistance = (dRadius(rng) + dRadius(rng)) * 0.5f;

        a.currentOrbitAngle = dAngle(rng);
        a.orbitalSpeed = 0.5f - 0.15f * ((a.orbitDistance - innerR) / (outerR - innerR));
        a.scale = dScale(rng);
        a.verticalOffset = dVert(rng);

        a.rotationAxis = glm::normalize(glm::vec3(dAxis(rng), dAxis(rng) + 1.5f, dAxis(rng)));
        a.selfRotationAngle = dAngle(rng);
        a.rotationSpeed = dRotSpd(rng);

        // Base color brownish grey
        glm::vec3 baseTint(0.45f, 0.38f, 0.33f);
        a.color = baseTint + glm::vec3(dColorVar(rng), dColorVar(rng), dColorVar(rng));
        a.color = glm::clamp(a.color, 0.0f, 1.0f);

        m_asteroids.push_back(a);
    }
}

void AsteroidBelt::update(float deltaTime)
{
    if (!m_enabled) return;
    for (auto& a : m_asteroids)
    {
        a.currentOrbitAngle += a.orbitalSpeed * deltaTime;
        a.selfRotationAngle += a.rotationSpeed * deltaTime;
        a.position.x = std::cos(a.currentOrbitAngle) * a.orbitDistance;
        a.position.z = std::sin(a.currentOrbitAngle) * a.orbitDistance;
    }
}

void AsteroidBelt::draw(core::Shader& shader, render::RenderStats* stats)
{
    if (!m_enabled || m_asteroids.empty()) return;

    // Fill dynamic instance buffer arrays (separated by mesh type)
    std::vector<float> meshBuffers[3];
    int instanceCounts[3] = {0, 0, 0};

    // Sort instances into 3 sub-arrays matching the 3 variation meshes
    for (size_t i = 0; i < m_asteroids.size(); ++i)
    {
        int meshIdx = i % 3;
        const auto& a = m_asteroids[i];

        glm::mat4 m = a.getModelMatrix();
        const float* pSource = (const float*)glm::value_ptr(m);
        for(int j=0; j<16; ++j) meshBuffers[meshIdx].push_back(pSource[j]);

        meshBuffers[meshIdx].push_back(a.color.r);
        meshBuffers[meshIdx].push_back(a.color.g);
        meshBuffers[meshIdx].push_back(a.color.b);
        meshBuffers[meshIdx].push_back(1.0f); // pad aligned

        instanceCounts[meshIdx]++;
    }

    shader.use();
    shader.setInt("uIsAsteroid", 1);

    for (int i = 0; i < 3; ++i)
    {
        if (instanceCounts[i] == 0) continue;

        glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, meshBuffers[i].size() * sizeof(float), meshBuffers[i].data());

        m_meshes[i]->drawInstanced(instanceCounts[i]);
        // Feature 1: record instanced draw call in the per-frame stats counter
        if (stats && instanceCounts[i] > 0)
            stats->recordInstancedDraw(instanceCounts[i], m_meshes[i]->indexCount());
    }

    shader.setInt("uIsAsteroid", 0);
}

} // namespace scene
