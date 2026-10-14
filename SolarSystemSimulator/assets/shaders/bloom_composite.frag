#version 330 core
// =============================================================================
// bloom_composite.frag  —  Tone-map HDR + additive bloom composite.
//
// Reads the full-resolution HDR scene and the blurred bloom texture,
// optionally adds bloom, then applies exposure-adjusted Reinhard tone-mapping
// and gamma correction before writing to the default (LDR) framebuffer.
// =============================================================================

in  vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uHdrScene;       ///< Full-resolution HDR scene (RGBA16F)
uniform sampler2D uBloom;          ///< Blurred bright-pass result (half-res, upsampled by GPU)
uniform float     uExposure;       ///< Reinhard exposure multiplier
uniform float     uBloomIntensity; ///< Additive bloom strength
uniform int       uBloomEnabled;   ///< 1 = apply bloom, 0 = tone-map only

void main()
{
    const float gamma = 2.2;

    vec3 hdr   = texture(uHdrScene, vTexCoord).rgb;
    vec3 bloom = texture(uBloom,    vTexCoord).rgb;

    if (uBloomEnabled == 1)
        hdr += bloom * uBloomIntensity;

    // Exposure-adjusted Reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdr * uExposure);

    // Gamma correction (linear → sRGB)
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);
}
