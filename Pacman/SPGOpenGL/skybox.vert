#version 400

// Fetele cubemap-ului
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;

    // Anulam translatia
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    
    // Eliminam efectul de perspectiva
    gl_Position = pos.xyww; 
}