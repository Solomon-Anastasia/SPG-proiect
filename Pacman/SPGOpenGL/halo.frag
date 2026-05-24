#version 400

in vec2 uv;

uniform vec3 haloColor;

out vec4 fragColor;

void main()
{
    float dist = length(uv);
    
    if (dist > 1.0) discard; // Eliminam pixelii care sunt in afara cercului de raza 1.0
    
    // Cu cat distanta e mai mare, cu atat pixelul e mai transparent
    float alpha = pow(1.0 - dist, 2.0); 
    
    fragColor = vec4(haloColor, alpha);
}