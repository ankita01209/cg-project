#version 330 core
// =============================================================================
// bloom_brightness.frag  —  Bright-pass luminance extraction.
//
// Samples the full-resolution HDR scene texture and outputs only pixels
// whose perceived luminance (Rec. 709 luma) exceeds uThreshold.
// Dark pixels are written as black so they contribute nothing to the blur.
// =============================================================================

in  vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uHdrScene;
uniform float     uThreshold;

void main()
{
    vec3  col        = texture(uHdrScene, vTexCoord).rgb;
    // Rec. 709 luminance coefficients
    float brightness = dot(col, vec3(0.2126, 0.7152, 0.0722));

    if (brightness > uThreshold)
        FragColor = vec4(col, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
