#version 400
out vec4 fragColor;

in vec3 normal;
in vec3 pos;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

in vec3 TangentLightPos;
in vec3 TangentViewPos;
in vec3 TangentFragPos;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 objectColor;

uniform bool useLighting;
uniform bool useTexture;
uniform bool useNormalMapping;
uniform bool useShadows;

uniform sampler2D wallTexture;
uniform sampler2D normalMap;
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir) 
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF (Percentage Closer Filtering) pentru a reduce aliasing-ul umbrilor
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
    vec3 texColor = texture(wallTexture, TexCoord).rgb;
    
    vec3 wallTint = vec3(0.4, 0.4, 0.65); // Brighter base color

    vec3 baseColor = useTexture ? (texColor * wallTint) : objectColor;

    if (!useLighting) 
    {
        fragColor = vec4(baseColor, 1.0);
        return;
    }

    vec3 N;
    if (useTexture && useNormalMapping) 
    {
        N = texture(normalMap, TexCoord).rgb;
        N = N * 2.0 - 1.0; 
        
        // Amplificam componentele X si Y pentru a face detaliile mai vizibile
        N.xy *= 1.5; 
        
        N = normalize(N); 
    } 
    else 
    {
        N = normalize(normal);
    }

    vec3 lightDir = (useTexture && useNormalMapping) ? normalize(TangentLightPos - TangentFragPos) : normalize(lightPos - pos);
    vec3 viewDir  = (useTexture && useNormalMapping) ? normalize(TangentViewPos - TangentFragPos) : normalize(viewPos - pos);

    // Ambient
    vec3 ambientComponent = vec3(0.25) * baseColor;

    // Lumina principala
    float dotLN = max(dot(N, lightDir), 0.0);
    vec3 diffuseMain = vec3(0.8) * dotLN * baseColor;
    
    // Specular pentru lumina principala, amplificat pentru a fi vizibil chiar si pe pereti texturati
    vec3 R1 = reflect(-lightDir, N);
    float spec1 = pow(max(dot(R1, viewDir), 0.0), 32.0);
    vec3 specularMain = vec3(0.3) * spec1;

    // Lumina pentru marirea detaliilor peretilor
    vec3 diffuseHeadlamp = vec3(0.0);
    vec3 specularHeadlamp = vec3(0.0);
    
    if (useTexture) 
    {
        vec3 headlampDir = viewDir; 
        
        float headlampDot = max(dot(N, headlampDir), 0.0);
        vec3 R2 = reflect(-headlampDir, N);

        float spec2 = pow(max(dot(R2, viewDir), 0.0), 32.0); 
        
        diffuseHeadlamp = vec3(0.1) * headlampDot * baseColor;
        specularHeadlamp = vec3(0.25) * spec2;               
    }

    // Umbra pentru lumina principala
    vec3 worldNormal = normalize(normal);
    vec3 worldLightDir = normalize(lightPos - pos);
    float shadow = useShadows ? ShadowCalculation(FragPosLightSpace, worldNormal, worldLightDir) : 0.0;

    // Aplicam umbra doar la lumina principala
    vec3 mainLightFinal = (1.0 - shadow) * (diffuseMain + specularMain);
    vec3 finalColor = ambientComponent + mainLightFinal + diffuseHeadlamp + specularHeadlamp;
    
    fragColor = vec4(finalColor, 1.0);
}