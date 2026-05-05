#version 400

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;

// Normala pentru pereti
layout(location = 2) in vec3 vNormal;

uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;
uniform mat4 modelMatrix;

out vec3 normal;
out vec3 pos;
out vec2 TexCoord;

uniform mat4 lightSpaceMatrix;
out vec4 FragPosLightSpace;

void main()
{
    gl_Position = mvpMatrix * vec4(vPos, 1.0);

    // Daca nu avem o normala valida, folosim pozitia ca normala (pentru sfere)
    vec3 actualNormal = length(vNormal) > 0.1 ? vNormal : normalize(vPos);
    
    normal = mat3(normalMatrix) * actualNormal;
    pos = vec3(modelMatrix * vec4(vPos, 1.0));

    TexCoord = vTexCoord;

    FragPosLightSpace = lightSpaceMatrix * vec4(pos, 1.0);
}