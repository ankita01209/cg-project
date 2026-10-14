#version 330 core
// =============================================================================
// basic.vert — Phase 3: separate model / view / projection uniforms
//
// CG concept – Vertex shader & MVP decomposition:
//   Splitting the single uMVP into three separate matrices lets us:
//     • Reuse the same view+projection for many objects (just swap uModel)
//     • Transform normals correctly (uses the model matrix only,
//       via the normal matrix = transpose(inverse(model)))
//     • Compute world-space position for future lighting calculations
// =============================================================================

// ── Vertex attributes ─────────────────────────────────────────────────────────
layout(location = 0) in vec3 aPos;       // Object-space position
layout(location = 1) in vec3 aNormal;    // Object-space surface normal
layout(location = 2) in vec2 aTexCoord;  // UV (reserved for textures)
layout(location = 3) in mat4 aInstanceMatrix;
layout(location = 7) in vec3 aInstanceColor;

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec3 vNormal;      // World-space normal (for shading / visualisation)
out vec3 vFragPos;     // World-space fragment position (future: lighting)
out vec2 vTexCoord;
out vec3 vInstanceColor;

// ── Uniforms ──────────────────────────────────────────────────────────────────
// CG concept – Matrix stack:
//   uModel      : object → world space
//   uView       : world → camera/eye space   (built with glm::lookAt)
//   uProjection : camera → clip space        (built with glm::perspective)
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform int  uIsAsteroid;

void main()
{
    mat4 modelMat = (uIsAsteroid == 1) ? aInstanceMatrix : uModel;

    // World-space position (needed for lighting in later phases)
    vec4 worldPos = modelMat * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;

    vNormal   = mat3(transpose(inverse(modelMat))) * aNormal;
    vTexCoord = aTexCoord;
    vInstanceColor = aInstanceColor;

    // Final clip-space position. Order matters: P * V * M * vertex.
    gl_Position = uProjection * uView * worldPos;
}
