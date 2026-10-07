#version 330 core
// =============================================================================
// unlit.frag
// =============================================================================

in vec2 vTexCoord;
out vec4 FragColor;

uniform vec4 uColor;
uniform int uIsBillboard;

void main()
{
    float alpha = 1.0;
    if (uIsBillboard == 1)
    {
        // 2D distance from center (0.5, 0.5) to current UV
        // Used to create a soft spherical-looking gradient on a flat quad
        float d = distance(vTexCoord, vec2(0.5));
        
        // smoothstep(edge0, edge1, x) -> if d > 0.5 returns 0, if d < 0 returned 1
        // Smoothly interpolates the alpha curve.
        alpha = smoothstep(0.5, 0.0, d);
    }

    FragColor = vec4(uColor.rgb, uColor.a * alpha);
}
