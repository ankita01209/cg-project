#version 330 core
// =============================================================================
// unlit.vert — Minimal shader for orbit rings, starfield, and glow billboards
// =============================================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;    // Not always used, but keeps VAO stride identical if needed
layout(location = 2) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// For billboard, we might nullify the view rotation.
uniform int uIsBillboard;

void main()
{
    vTexCoord = aTexCoord;

    if (uIsBillboard == 1)
    {
        // CG concept: Spherical Billboarding
        // By taking the View matrix (which rotates the world around the camera)
        // and setting its upper 3x3 rotation portion to Identity, the object
        // ALWAYS faces the camera exactly.
        mat4 modelView = uView * uModel;
        modelView[0][0] = 1.0; modelView[0][1] = 0.0; modelView[0][2] = 0.0;
        modelView[1][0] = 0.0; modelView[1][1] = 1.0; modelView[1][2] = 0.0;
        modelView[2][0] = 0.0; modelView[2][1] = 0.0; modelView[2][2] = 1.0;
        
        // However, since we might scale the billboard using uModel, we need to 
        // extract the scale first. Or simpler: just build the translation directly.
        // It's cleaner to just discard view rotation entirely:
        mat4 viewZeroRot = uView;
        viewZeroRot[0][0] = 1.0; viewZeroRot[0][1] = 0.0; viewZeroRot[0][2] = 0.0;
        viewZeroRot[1][0] = 0.0; viewZeroRot[1][1] = 1.0; viewZeroRot[1][2] = 0.0;
        viewZeroRot[2][0] = 0.0; viewZeroRot[2][1] = 0.0; viewZeroRot[2][2] = 1.0;

        gl_Position = uProjection * viewZeroRot * uModel * vec4(aPos, 1.0);
    }
    else
    {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
}
