#version 400

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;

// Normala pentru pereti
layout(location = 2) in vec3 vNormal;

layout(location = 3) in vec3 vTangent;

uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightSpaceMatrix;

uniform vec3 lightPos;
uniform vec3 viewPos;

out vec3 normal;
out vec3 pos;
out vec2 TexCoord;

out vec3 TangentLightPos;
out vec3 TangentViewPos;
out vec3 TangentFragPos;

out vec4 FragPosLightSpace;

void main()
{
    gl_Position = mvpMatrix * vec4(vPos, 1.0);
    pos = vec3(modelMatrix * vec4(vPos, 1.0));
    TexCoord = vTexCoord;
    FragPosLightSpace = lightSpaceMatrix * vec4(pos, 1.0);

    // Verificam daca normala si tangenta sunt valide, daca nu, folosim valori implicite
    vec3 actualNormal = length(vNormal) > 0.1 ? vNormal : normalize(vPos);
    vec3 actualTangent = length(vTangent) > 0.1 ? vTangent : vec3(1.0, 0.0, 0.0);

    vec3 T = normalize(vec3(normalMatrix * vec4(actualTangent, 0.0)));
    vec3 N = normalize(vec3(normalMatrix * vec4(actualNormal, 0.0)));
    
    // Procesul de ortogonalizare Gram-Schmidt pentru a asigura ca T este perpendicular pe N
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T); 

    mat3 TBN = transpose(mat3(T, B, N)); 

    TangentLightPos = TBN * lightPos;
    TangentViewPos = TBN * viewPos;
    TangentFragPos = TBN * pos;
    normal = N;
}