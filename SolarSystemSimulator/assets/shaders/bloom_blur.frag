#version 330 core
// =============================================================================
// bloom_blur.frag  —  Separable 9-tap Gaussian blur (horizontal OR vertical).
//
// Set uHorizontal = 1 for the horizontal pass, 0 for the vertical pass.
// Ping-pong between two half-resolution FBOs for N iterations.
//
// Kernel weights (sigma ≈ 1.0, 5 taps each side = 9-tap total):
//   centre: 0.227027,  tap1: 0.194595,  tap2: 0.121622,
//   tap3  : 0.054054,  tap4: 0.016216
// Sum across all taps: 0.227027 + 2*(0.194595+0.121622+0.054054+0.016216) = 1.0
// =============================================================================

in  vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uImage;
uniform int       uHorizontal;  // 1 = horizontal, 0 = vertical

const float weights[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(uImage, 0));
    vec3 result    = texture(uImage, vTexCoord).rgb * weights[0];

    if (uHorizontal == 1)
    {
        for (int i = 1; i < 5; ++i)
        {
            vec2 offset = vec2(texelSize.x * float(i), 0.0);
            result += texture(uImage, vTexCoord + offset).rgb * weights[i];
            result += texture(uImage, vTexCoord - offset).rgb * weights[i];
        }
    }
    else
    {
        for (int i = 1; i < 5; ++i)
        {
            vec2 offset = vec2(0.0, texelSize.y * float(i));
            result += texture(uImage, vTexCoord + offset).rgb * weights[i];
            result += texture(uImage, vTexCoord - offset).rgb * weights[i];
        }
    }

    FragColor = vec4(result, 1.0);
}
