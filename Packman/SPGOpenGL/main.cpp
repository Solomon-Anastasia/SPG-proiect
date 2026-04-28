#include <iostream>
#include <fstream>
#include <string>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include "spheremesh.h"
#include "cubeMesh.h"

#define PI glm::pi<float>()

GLuint shader_programme, vao;

glm::mat4 projectionMatrix, viewMatrix, modelMatrix;

glm::vec3 lightPos(0, 1, 5);
glm::vec3 viewPos(5.0f, 18.0f, 12.0f);

SphereMesh sphere(1);
GLuint sphereVao, sphereVbo, sphereEbo;
int sphereElementCount = (GLsizei)sphere.triangles.size() * sizeof(glm::ivec3);

CubeMesh wallCube;
GLuint cubeVao, cubeVbo, cubeEbo;
int cubeElementCount = (GLsizei)wallCube.triangles.size() * 3;

// 1 = Wall, 0 = Path, 2 = Pellet
const int MAZE_WIDTH = 20;
const int MAZE_HEIGHT = 15;
int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1},
	{1,0,1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,1,1,0,1,0,1,1,1,1,1,1,0,1,0,1,1,0,1},
	{1,0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,1},
	{1,1,1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1,1,1},
	{1,1,1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,1,1,1},
	{1,0,0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0,0,1},
	{1,0,1,1,1,1,0,1,0,0,0,0,1,0,1,1,1,1,0,1},
	{1,0,0,0,0,1,0,1,1,1,1,1,1,0,1,0,0,0,0,1},
	{1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1},
	{1,0,1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1,0,1},
	{1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

float pacX = 1.0f;
float pacZ = 1.0f;

float pacAngle = 0.0f; // Rotation angle in radians

float targetX = 1.0f;
float targetZ = 1.0f;
float smoothSpeed = 0.01f;

float targetAngle = 0.0f; // Where we want to face
float currentAngle = 0.0f; // Where we are currently facing
float rotationSpeed = 0.05f; // Adjust for "snappiness"

int lastMouseX = -1;
float cameraYaw = 0.0f; // extra yaw from mouse drag
bool mouseDown = false;

void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		mouseDown = (state == GLUT_DOWN);
		lastMouseX = x;
	}
}

void motion(int x, int y)
{
	if (mouseDown && lastMouseX >= 0) {
		float delta = (x - lastMouseX) * 0.01f;
		cameraYaw += delta;
		lastMouseX = x;
	}
}

std::string textFileRead(char* fn)
{
	std::ifstream ifile(fn);
	std::string filetext;
	while (ifile.good()) {
		std::string line;
		std::getline(ifile, line);
		filetext.append(line + "\n");
	}
	return filetext;
}

void printShaderInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0)
	{
		infoLog = (char*)malloc(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		free(infoLog);
	}
}

void printProgramInfoLog(GLuint obj)
{
	int infologLength = 0;
	int charsWritten = 0;
	char* infoLog;

	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);

	if (infologLength > 0)
	{
		infoLog = (char*)malloc(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		free(infoLog);
	}
}

float rotAngle = 0;
float rotAngleInc = PI / 64;

void display()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shader_programme);

	pacX += (targetX - pacX) * smoothSpeed;
	pacZ += (targetZ - pacZ) * smoothSpeed;

	currentAngle += (targetAngle - currentAngle) * rotationSpeed;

	float distanceBehind = 4.0f; // Distance from Pac-Man
	float heightAbove = 3.0f;    // How "upper" the camera is

	// We use sin/cos of the current angle to maintain the "follow" distance
	float camAngle = currentAngle + cameraYaw;
	float camX = pacX + sin(camAngle) * distanceBehind;
	float camZ = pacZ + cos(camAngle) * distanceBehind;

	viewPos = glm::vec3(camX, heightAbove, camZ);

	// Look-at point: Pac-Man's center
	viewMatrix = glm::lookAt(viewPos, glm::vec3(pacX, 0.5f, pacZ), glm::vec3(0, 1, 0));

	/*viewPos = glm::vec3(pacX, 1.5f, pacZ + 3.5f);
	viewMatrix = glm::lookAt(viewPos, glm::vec3(pacX, 0.5f, pacZ - 2.0f), glm::vec3(0, 1, 0));*/


	// 1. SET GLOBAL UNIFORMS
	GLint colorLoc = glGetUniformLocation(shader_programme, "objectColor");
	GLint useLightLoc = glGetUniformLocation(shader_programme, "useLighting");

	GLuint lightPosLoc = glGetUniformLocation(shader_programme, "lightPos");
	glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
	GLuint viewPosLoc = glGetUniformLocation(shader_programme, "viewPos");
	glUniform3fv(viewPosLoc, 1, glm::value_ptr(viewPos));

	GLuint modelMatrixLoc = glGetUniformLocation(shader_programme, "mvpMatrix");
	GLuint normalMatrixLoc = glGetUniformLocation(shader_programme, "normalMatrix");

	modelMatrix = glm::translate(glm::vec3(pacX, 0, pacZ));
	modelMatrix = glm::rotate(modelMatrix, currentAngle, glm::vec3(0, 1, 0)); // Add rotation
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));

	// 2. DRAW THE MAZE WALLS
	glUniform1i(useLightLoc, 0);          // Lighting OFF
	glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f); // SLIGHTLY GREY SO YOU SEE EDGES

	glBindVertexArray(cubeVao);
	for (int r = 0; r < MAZE_HEIGHT; r++) {
		for (int c = 0; c < MAZE_WIDTH; c++) {
			if (maze[r][c] == 1) {
				modelMatrix = glm::translate(glm::vec3(c, 0, r));

				modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.5f, 1.0f));

				glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
				// Normal matrix isn't strictly needed if lighting is OFF, but good practice:
				glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelMatrix))));

				glDrawElements(GL_TRIANGLES, cubeElementCount, GL_UNSIGNED_INT, NULL);
			}
		}
	}

	// 3. DRAW PAC-MAN
	glUniform1i(useLightLoc, 1);          // Lighting ON
	glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // Yellow

	glBindVertexArray(sphereVao);

	// Position Pac-Man at a starting spots
	modelMatrix = glm::translate(glm::vec3(pacX, 0, pacZ));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.3f, 0.3f, 0.3f));

	// Normals for sphere
	glm::mat4 pacNormalMatrix = glm::transpose(glm::inverse(modelMatrix));

	glUniformMatrix4fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(pacNormalMatrix));
	glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));

	glDrawElements(GL_TRIANGLES, sphereElementCount, GL_UNSIGNED_INT, NULL);

	glFlush();
	glutPostRedisplay();
}

void init()
{
	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	glClearColor(1, 1, 1, 0);
	glEnable(GL_DEPTH_TEST);

	glewInit();

	// 1. SPHERE INITIALIZATION
	glGenBuffers(1, &sphereVbo);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVbo);
	glBufferData(GL_ARRAY_BUFFER, sphere.vertices.size() * sizeof(glm::vec3), sphere.vertices.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &sphereVao);
	glBindVertexArray(sphereVao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glGenBuffers(1, &sphereEbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereElementCount, sphere.triangles.data(), GL_STATIC_DRAW);

	// 2. CUBE INITIALIZATION
	glGenVertexArrays(1, &cubeVao);
	glBindVertexArray(cubeVao);

	glGenBuffers(1, &cubeVbo);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
	glBufferData(GL_ARRAY_BUFFER, wallCube.vertices.size() * sizeof(glm::vec3), wallCube.vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glGenBuffers(1, &cubeEbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, wallCube.triangles.size() * sizeof(glm::ivec3), wallCube.triangles.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	std::string vstext = textFileRead("pixel_light.vert");
	std::string fstext = textFileRead("pixel_light.frag");

	const char* vertex_shader = vstext.c_str();
	const char* fragment_shader = fstext.c_str();

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);

	shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	printShaderInfoLog(shader_programme);
	printProgramInfoLog(shader_programme);
}

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	projectionMatrix = glm::perspective(PI / 4, (float)w / h, 0.1f, 100.0f);
	/*
	viewMatrix este matricea transformarii de observare. Parametrii functiei
	lookAt sunt trei vectori ce reprezinta, in ordine:
	- pozitia observatorului
	- punctul catre care priveste observatorul
	- directia dupa care este orientat observatorul
	*/
	/*viewPos = glm::vec3(5.0f, 12.0f, 14.0f);
	viewMatrix = glm::lookAt(viewPos, glm::vec3(5.0f, 0, 5.0f), glm::vec3(0, 1, 0));*/
}

void keyboard(unsigned char key, int x, int y)
{
	// Direction vectors based on current facing angle
	float dx = sin(targetAngle);
	float dz = cos(targetAngle);

	float nextX = targetX;
	float nextZ = targetZ;

	switch (key)
	{
	case 's': nextX -= dx; nextZ -= dz; break; // Forward
	case 'w': nextX += dx; nextZ += dz; break; // Backward
	case 'd': targetAngle -= glm::radians(90.0f); return; // Turn left
	case 'a': targetAngle += glm::radians(90.0f); return; // Turn right
	}

	int gridX = (int)(nextX + 0.5f);
	int gridZ = (int)(nextZ + 0.5f);

	if (gridX >= 0 && gridX < MAZE_WIDTH && gridZ >= 0 && gridZ < MAZE_HEIGHT)
		if (maze[gridZ][gridX] != 1)
		{
			targetX = nextX;
			targetZ = nextZ;
		}

	glutPostRedisplay();
}

void specialKeyboard(int key, int x, int y)
{
	unsigned char mapped = 0;
	switch (key) {
	case GLUT_KEY_UP:    mapped = 'w'; break;
	case GLUT_KEY_DOWN:  mapped = 's'; break;
	case GLUT_KEY_LEFT:  mapped = 'a'; break;
	case GLUT_KEY_RIGHT: mapped = 'd'; break;
	}
	if (mapped) keyboard(mapped, x, y);
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_SINGLE);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(1000, 800);
	glutCreateWindow("Pack-man");

	init();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glutKeyboardFunc(keyboard);
	glutSpecialFunc(specialKeyboard);

	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();

	return 0;
}
