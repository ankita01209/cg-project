#version 330 core
// =============================================================================
// basic.frag — Phase 4: flat per-body colour with subtle normal shading
//
// CG concept – Flat shading vs. per-fragment lighting:
//   True Phong lighting (Phase 5) computes a diffuse/specular term based on
//   the angle between the surface normal and a light direction. Here we use a
//   lightweight approximation:
//     • base colour comes from a CPU-supplied uniform (uColor)
//     • a simple "fake ambient + front-face brightening" using the Z component
//       of the normal gives the sphere a 3D appearance without any light math
//
//   This is sometimes called "normal dot view" shading or "rim brightening".
//   It is NOT physically based but it IS visually convincing at this stage.
// =============================================================================

in  vec3 vNormal;
in  vec3 vFragPos;
in  vec2 vTexCoord;

out vec4 FragColor;

/// Per-draw-call colour set by the CPU for each celestial body
uniform vec3 uColor;

void main()
{
    // Re-normalise after interpolation (can drift slightly across a triangle)
    vec3 N = normalize(vNormal);

    // CG concept – Simple brightness from normal orientation:
    //   N.z is the Z component of the world-space normal. For a sphere facing
    //   the camera (+Z direction), front-facing normals have N.z ≈ +1
    //   (bright), edge normals have N.z ≈ 0, and back-facing ones are culled.
    //   Remapping to [0.25, 1.0] prevents the dark side from going fully black,
    //   giving a readable depth cue without a real light source.
    float brightness = N.z * 0.5 + 0.5;         // remap [-1,1] → [0, 1]
    brightness       = 0.30f + brightness * 0.70f; // clamp minimum to 0.30

    FragColor = vec4(uColor * brightness, 1.0);
}
