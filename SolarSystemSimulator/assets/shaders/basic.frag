#version 330 core
// =============================================================================
// basic.frag â€” Phase 4: flat per-body colour with subtle normal shading
//
// CG concept â€“ Flat shading vs. per-fragment lighting:
//   True Phong lighting (Phase 5) computes a diffuse/specular term based on
//   the angle between the surface normal and a light direction. Here we use a
//   lightweight approximation:
//     â€¢ base colour comes from a CPU-supplied uniform (uColor)
//     â€¢ a simple "fake ambient + front-face brightening" using the Z component
//       of the normal gives the sphere a 3D appearance without any light math
//
//   This is sometimes called "normal dot view" shading or "rim brightening".
//   It is NOT physically based but it IS visually convincing at this stage.
// =============================================================================

in  vec3 vNormal;
in  vec3 vFragPos;
in  vec2 vTexCoord;

out vec4 FragColor;

in vec3 vInstanceColor;
uniform int uIsAsteroid;

uniform sampler2D uTexture;
uniform int uHasTexture;
uniform int uGlobalTextureEnabled;
uniform float uAlpha;

/// Per-draw-call colour set by the CPU for each celestial body
uniform vec3 uColor;

// â”€â”€ Phase 8: Lighting Uniforms â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform int  uIsEmissive;
uniform float uSpecularStrength;
uniform float uShininess;

// ── Session 2 Addition 1: Rim-Highlight ─────────────────────────────────────
uniform int  uIsHighlighted;
uniform vec3 uHighlightColor;

void main()
{
    vec3 baseCol = (uIsAsteroid == 1) ? vInstanceColor : uColor;
    if (uHasTexture == 1 && uGlobalTextureEnabled == 1 && uIsAsteroid == 0) {
        baseCol = texture(uTexture, vTexCoord).rgb;
    }

    vec3 surfaceColor = baseCol;

    if (uIsEmissive == 1)
    {
        // The Sun is emissive and unlit.
        // It provides the light, so it shouldn't be shaded by its own light.
        FragColor = vec4(surfaceColor, uAlpha);
        return;
    }

    // CG concept – Blinn-Phong Lighting Model
    // 1. Ambient: constant baseline light so planet textures and colors remain vibrant and visible
    // 2. Diffuse: directional light scattered from the Sun (Lambertian)
    // 3. Specular: shiny highlight where light reflects directly into the camera

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    vec3 V = normalize(uViewPos - vFragPos);

    // 1. Ambient (boosted so surface textures and real planet colors are clearly visible)
    float ambientStrength = 0.28;
    vec3 ambient = ambientStrength * surfaceColor;

    // 2. Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * surfaceColor;

    // 3. Specular (configurable for metallic Psyche vs matte carbon Hygiea)
    float specStr = (uSpecularStrength > 0.0) ? uSpecularStrength : 0.25;
    float shininess = (uShininess > 0.0) ? uShininess : 32.0;

    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = specStr * spec * vec3(1.0);

    // Final composition
    vec3 finalColor = ambient + diffuse + specular;

    // Session 2 Addition 1: Selected Body Rim Highlight
    if (uIsHighlighted == 1)
    {
        // Fresnel factor: sharpest at the very edge where dot(N,V) nears 0
        float rimFactor = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        // Additive blend with highlight color
        finalColor += uHighlightColor * rimFactor;
    }

    FragColor = vec4(finalColor, uAlpha);
}
