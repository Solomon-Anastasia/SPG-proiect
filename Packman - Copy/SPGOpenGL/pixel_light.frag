#version 400
out vec4 fragColor;

in vec3 normal;
in vec3 pos;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 objectColor;
uniform bool useLighting;

vec3 lighting(vec3 pos, vec3 normal, vec3 lightPos, vec3 viewPos,
				vec3 ambient, vec3 diffuse, vec3 specular, float specPower)
{
	//functia calculeaza si returneaza culoarea conform cu modelul de iluminare Phong descris in documentatie
	vec3 N = normalize(normal);
    
    vec3 L = normalize(lightPos - pos);
    
    vec3 ambientComponent = ambient;
    
    float dotLN = max(dot(N, L), 0.0);
    vec3 diffuseComponent = diffuse * dotLN;
    
    vec3 V = normalize(viewPos - pos);
    
    vec3 R = reflect(-L, N);
    
    float spec = pow(max(dot(R, V), 0.0), specPower);
    vec3 specularComponent = specular * spec;
    
    return (ambientComponent + diffuseComponent) * objectColor + specularComponent;
}

void main() 
{
	if (!useLighting) {
        fragColor = vec4(objectColor, 1.0);
    } else {
        vec3 ambient = vec3(0.2);
        vec3 diffuse = vec3(0.8);
        vec3 specular = vec3(0.5);
        float specPower = 32;
        
        vec3 color = lighting(pos, normal, lightPos, viewPos, 
                    ambient, diffuse, specular, specPower);
        fragColor = vec4(color, 1.0);
    }
}
