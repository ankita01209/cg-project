#version 330 core
// =============================================================================
// atmosphere.frag
// Addition 4: Computes Fresnel rim lighting effect for Earth's atmosphere
// =============================================================================

in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uViewPos;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPos);
    
    // CG Concept: Fresnel Rim Effect
    // dot(N, V) is 1 at the center (facing eye) and 0 at the extreme grazing edges.
    // Inverting it via 1 - dot gives us 0 at center and 1 at edges.
    // Raising it to a power (fresnel power) sharpens the glow to just the rim.
    float NdotV = max(dot(N, V), 0.0);
    float fresnel = pow(1.0 - NdotV, 3.0);
    
    // Additive blending uses pre-multiplied alpha or just outputs alpha.
    FragColor = vec4(uColor, fresnel);
}
