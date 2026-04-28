#version 400
in vec3 vPos;

uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;

out vec3 normal;
out vec3 pos;

void main() 
{
	gl_Position = mvpMatrix * vec4(vPos, 1.0);
	
	//normalele se determina ca fiind vectori cu aceeasi directie ca si cea determinata de originea sferei si pozitia vPos
	normal = normalize(vPos);
	
	//normalele se inmultesc cu matricea normalMatrix
	normal = mat3(normalMatrix) * normal;
	
	pos = vPos;
}