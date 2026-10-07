#version 330 core
// =============================================================================
// atmosphere.vert
// Addition 4: Computes world-space standard vectors for Fresnel calculation
// =============================================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;
out vec3 vWorldPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    
    // CG Concept: Normal matrix
    // If the model matrix has non-uniform scaling, we'd need inverse(transpose(mat3(uModel))).
    // But since planets/atmospheres are uniformly scaled, mat3 is fine.
    vNormal = mat3(uModel) * aNormal;
    
    gl_Position = uProjection * uView * worldPos;
}
