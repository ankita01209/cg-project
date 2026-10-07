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

// ── Outputs to fragment shader ────────────────────────────────────────────────
out vec3 vNormal;      // World-space normal (for shading / visualisation)
out vec3 vFragPos;     // World-space fragment position (future: lighting)
out vec2 vTexCoord;

// ── Uniforms ──────────────────────────────────────────────────────────────────
// CG concept – Matrix stack:
//   uModel      : object → world space
//   uView       : world → camera/eye space   (built with glm::lookAt)
//   uProjection : camera → clip space        (built with glm::perspective)
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    // World-space position (needed for lighting in later phases)
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos  = worldPos.xyz;

    // CG concept – Normal matrix:
    //   When the model matrix contains non-uniform scaling, normals must be
    //   transformed by transpose(inverse(model)) to stay perpendicular to the
    //   surface. For uniform scaling (like our sphere) mat3(uModel) suffices,
    //   but the correct form is already here, ready for general use.
    vNormal   = mat3(transpose(inverse(uModel))) * aNormal;

    vTexCoord = aTexCoord;

    // Final clip-space position. Order matters: P * V * M * vertex.
    gl_Position = uProjection * uView * worldPos;
}
