#version 330 core
// =============================================================================
// fullscreen.vert  —  Shared vertex shader for all post-processing passes.
//
// Renders a screen-space quad using two NDC triangles (6 vertices from a VBO).
// No model/view/projection matrices needed — positions are already in NDC.
// Attribute layout matches the Bloom quad VBO (pos xy, texcoord xy).
// =============================================================================

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    vTexCoord   = aTexCoord;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
