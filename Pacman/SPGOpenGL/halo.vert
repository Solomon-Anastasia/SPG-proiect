#version 400

layout(location = 0) in vec2 vPos;

uniform mat4 projection;
uniform mat4 view;

uniform vec3 centerPos;

uniform float size;

out vec2 uv;

void main()
{
    // Transmitem pozitia in spatiu a vertexului pentru fragment shader 
    uv = vPos;
    
    // Halo mereu spre camera
    vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 up = vec3(view[0][1], view[1][1], view[2][1]);
    
    // Calculam pozitia in lume a fiecarui vertex al halo-ului
    vec3 worldPos = centerPos + right * vPos.x * size + up * vPos.y * size;
    
    gl_Position = projection * view * vec4(worldPos, 1.0);
}