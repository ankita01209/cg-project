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

// â”€â”€ Session 2 Addition 1: Rim-Highlight â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
uniform int  uIsHighlighted;
uniform vec3 uHighlightColor;

void main()
{
    // Phase 5: Debug stripe to prove rotation is working.
    // vTexCoord.x is longitude, circling the sphere 0.0 -> 1.0.
    // Kept faint so we can still see rotation even on lit uniform spheres.
    float isStripe = step(0.96, fract(vTexCoord.x * 4.0));

    vec3 baseCol = (uIsAsteroid == 1) ? vInstanceColor : uColor;
    if (uHasTexture == 1 && uGlobalTextureEnabled == 1 && uIsAsteroid == 0) {
        baseCol = texture(uTexture, vTexCoord).rgb;
    }

    vec3 surfaceColor = mix(baseCol, vec3(1.0), isStripe * 0.15); // faint stripe

    if (uIsEmissive == 1)
    {
        // The Sun is emissive and unlit.
        // It provides the light, so it shouldn't be shaded by its own light.
        FragColor = vec4(surfaceColor, uAlpha);
        return;
    }

    // CG concept â€“ Blinn-Phong Lighting Model
    // 1. Ambient: constant low-level light simulating indirect bounce
    // 2. Diffuse: directional light scattered in all directions (Lambertian)
    // 3. Specular: shiny highlight where light reflects directly into the camera

    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vFragPos);
    vec3 V = normalize(uViewPos - vFragPos);

    // 1. Ambient
    float ambientStrength = 0.05;
    vec3 ambient = ambientStrength * surfaceColor;

    // 2. Diffuse
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * surfaceColor;

    // 3. Specular (Blinn-Phong uses Halfway vector H)
    float specularStrength = 0.3;
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 32.0); // 32 is shininess
    // We colour the specular highlight slightly towards white
    vec3 specular = specularStrength * spec * vec3(1.0);

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
