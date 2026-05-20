#version 400
out vec4 fragColor;

in vec3 normal;
in vec3 pos;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 objectColor;

uniform bool useLighting;
uniform bool useTexture;

uniform sampler2D wallTexture;

uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) 
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // Reduced PCF: Only 4 samples instead of 9
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = 0; x <= 1; ++x) 
    {
        for(int y = 0; y <= 1; ++y) 
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }    
    }
    return shadow / 4.0;
}

vec3 lighting(vec3 pos, vec3 normal, vec3 lightPos, vec3 viewPos,
              vec3 ambient, vec3 diffuse, vec3 specular, float specPower,
              vec3 baseColor)
{
    vec3 N = normalize(normal);
    vec3 L = normalize(lightPos - pos);

    vec3 ambientComponent = ambient;

    float dotLN = max(dot(N, L), 0.0);
    vec3 diffuseComponent = diffuse * dotLN;

    vec3 V = normalize(viewPos - pos);
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(R, V), 0.0), specPower);
    
    // Eliminam componenta speculara pentru pereti texturati
    vec3 specularComponent = useTexture ? vec3(0.0) : (specular * spec);

    float shadow = ShadowCalculation(FragPosLightSpace, N, L);

    // Aplicam iluminarea doar daca pixelul nu este in umbra
    return (ambientComponent * baseColor) + (1.0 - shadow) * ((diffuseComponent * baseColor) + specularComponent);
}

void main()
{
    vec3 baseColor;

    if (useTexture) 
    {
        baseColor = texture(wallTexture, TexCoord).rgb;
    } 
    else 
    {
        baseColor = objectColor;
    }

    if (!useLighting) 
    {
        fragColor = vec4(baseColor, 1.0);
    } 
    else 
    {
        vec3 ambient = vec3(0.4);
        vec3 diffuse = vec3(0.8);
        vec3 specular = vec3(0.3);
        float specPower = 16.0;
        
        vec3 color = lighting(pos, normal, lightPos, viewPos, ambient, diffuse, specular, specPower, baseColor);
        fragColor = vec4(color, 1.0);
    }
}