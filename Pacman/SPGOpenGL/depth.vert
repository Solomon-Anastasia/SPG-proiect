#version 400

layout (location = 0) in vec3 vPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 modelMatrix;

void main() 
{
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(vPos, 1.0);
}