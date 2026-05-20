#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/mat4x4.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

#include <queue>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "objloader.hpp"

#include "spheremesh.h"
#include "cubeMesh.h"

#define PI glm::pi<float>()

GLuint shader_programme, vao;
glm::mat4 projectionMatrix, viewMatrix, modelMatrix;

SphereMesh sphere(3);
GLuint sphereVao, sphereVbo, sphereEbo;
int sphereElementCount = (GLsizei)sphere.triangles.size() * sizeof(glm::ivec3);

CubeMesh wallCube;
GLuint cubeVao, cubeVbo, cubeEbo;

// Fiecare cub are 12 triunghiuri, deci 36 indici
int cubeElementCount = (GLsizei)wallCube.triangles.size() * 3;

GLuint wallBoxVao, wallBoxVbo;
GLuint wallTexture;

//glm::vec3 lightPos(10.0f, 8.0f, 7.0f); // Pozitia sursei de lumina
glm::vec3 lightPos(10.0f, 20.0f, 15.0f);
glm::vec3 viewPos(5.0f, 18.0f, 12.0f); // Pozitia initiala a camerei

// Pentru shadow mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // Rezolutia
GLuint depth_shader_programme; // Avem nevoie de un shader separat pentru a desena in depth map

// Logica pentru lerping pentru pacman si camera
float pacX = 9.0f, pacZ = 10.0f;
float targetX = 9.0f, targetZ = 10.0f;

// Schimbam unghiul initial la PI ca sa priveasca in sus (spre un culoar liber)
float currentAngle = PI, targetAngle = PI;
float rotationSpeed = 0.01f;

float cameraAngle = PI, cameraTarget = PI;
float cameraLagSpeed = 0.01f;

float lastFrameTime = 0.0f;
float deltaTime = 0.0f;

// Pentru controlul camerei cu mouse-ul
int lastMouseX = -1;
float cameraOrbit = 0.0f;
bool mouseDown = false;

// 1 = perete, 0 = cale
const int MAZE_WIDTH = 20;
const int MAZE_HEIGHT = 15;
int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1},
    {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0}, // Tunel
    {1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1},
    {1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};


bool pellets[MAZE_HEIGHT][MAZE_WIDTH];

// Salvam vertexii pentru a reduce draw call-urile pentru fiecare cub din labirint
struct Vertex 
{
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;
};

struct WallSegment
{
    float x, z;
    float width, depth;
};


std::vector<WallSegment> wallSegments;
std::vector<Vertex> allWallVertices;

// Fantome
std::vector<glm::vec3> ghostVertices;
std::vector<glm::vec3> ghostNormals;
std::vector<glm::vec2> ghostUVs;
GLuint ghostVao, ghostVboPos, ghostVboNorm;

struct Ghost
{
    float x, z;
    float targetX, targetZ;
    glm::vec3 color;
    float speed = 0.3f;

    int currentDir;
    float targetAngle;
    float currentAngle;

    // Patrulare
    std::vector<glm::ivec2> route;
    int routeIndex = 0;
};


struct ShaderUniforms {
    GLint mvpLoc = -1;
    GLint modelLoc = -1;
    GLint normalLoc = -1;
    GLint colorLoc = -1;
    GLint useLightLoc = -1;
    GLint useTexLoc = -1;

    GLint lightSpaceLoc = -1;
    GLint lightPosLoc = -1;
    GLint viewPosLoc = -1;
    GLint textureLoc = -1;
    GLint shadowMapLoc = -1;
};

ShaderUniforms lightUniforms;   // pentru shader_programme
ShaderUniforms depthUniforms;   // pentru depth_shader_programme

std::vector<Ghost> ghosts;

void initGhosts()
{
    ghosts.clear();
    glm::vec3 colors[] =
    {
        glm::vec3(1.0f, 0.0f, 0.0f), // Blinky (top sus)
        glm::vec3(1.0f, 0.7f, 0.8f), // Pinky (top dreapta)
        glm::vec3(0.0f, 1.0f, 1.0f), // Inky (stanga jos)
        glm::vec3(1.0f, 0.5f, 0.0f)  // Clyde (stanga sus)
    };

    std::vector<std::vector<glm::ivec2>> routes =
    {
        {{9, 1}, {9, 3}, {6, 3}, {6, 8}, {13, 8}, {13, 3}, {10, 3}, {10, 1}},
        {{4, 1}, {4, 10}, {9, 10}, {9, 8}, {10, 8}, {10, 10}, {15, 10}, {15, 1}},
        {{1, 10}, {1, 12}, {8, 12}, {8, 13}, {11, 13}, {11, 12}, {18, 12}, {18, 10}, {10, 10}, {10, 8}, {9, 8}, {9, 10}},
        {{9, 5}, {8, 5}, {8, 6}, {11, 6}, {11, 5}, {10, 5}, {10, 10}, {18, 10}, {18, 1}, {10, 1}, {10, 3}, {9, 3}, {9, 1}, {1, 1}, {1, 10}, {9, 10}}
    };

    for (int i = 0; i < 4; i++)
    {
        Ghost g;
        g.route = routes[i];
        g.routeIndex = 0;

        // Incepem fiecare fantoma la prima pozitie din ruta sa
        g.x = g.targetX = (float)g.route[0].x;
        g.z = g.targetZ = (float)g.route[0].y;

        g.color = colors[i];
        g.speed = 0.06f;

        g.currentDir = 3; // Privire initiala in sus pentru toate fantomele
        g.targetAngle = 0.0f;
        g.currentAngle = 0.0f;

        ghosts.push_back(g); // Adaugam fantoma in lista globala
    }
}

void updateGhosts(float dt)
{
    for (auto& g : ghosts) // & pentru a modifica direct in vectorul global
    {
        // Calculam distanta pana la tinta curenta
        float dx = g.targetX - g.x;
        float dz = g.targetZ - g.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        // Daca distanta e semnificativa, continuam sa ne miscam catre tinta
        if (dist > 0.001f)
        {
            float step = g.speed * 60.0f * dt;

            if (step > dist) step = dist;

            g.x += (dx / dist) * step;
            g.z += (dz / dist) * step;
        }

        // Lerp unghiular pentru a face intoarcerile mai fluide
        float angleDiff = g.targetAngle - g.currentAngle;
        while (angleDiff > PI) angleDiff -= 2.0f * PI;
        while (angleDiff < -PI) angleDiff += 2.0f * PI;

        g.currentAngle += angleDiff * (10.0f * dt);

        // Daca am ajuns aproape de tinta, trecem la urmatorul punct din ruta
        if (dist <= 0.01f) 
        {
            g.x = g.targetX;
            g.z = g.targetZ;

            // Avansam in ruta
            g.routeIndex = (g.routeIndex + 1) % g.route.size();
            glm::ivec2 nextStep = g.route[g.routeIndex];

            g.targetX = (float)nextStep.x;
            g.targetZ = (float)nextStep.y;

            // Determinam noua tinta unghiulara bazata pe directia de miscare
            int roundedX = (int)(g.x + 0.5f);
            int roundedZ = (int)(g.z + 0.5f);

            if (nextStep.x > roundedX)
            {
                g.targetAngle = 0.0f; // Fata dreapta
            }
            else if (nextStep.x < roundedX)
            {
                g.targetAngle = PI; // Fata stanga
            }
            else if (nextStep.y > roundedZ)
            {
                g.targetAngle = -PI / 2.0f; // Fata jos
            }
            else if (nextStep.y < roundedZ)
            {
                g.targetAngle = PI / 2.0f; // Fata sus
            }
        }
    }
}

void checkGameOver()
{
    for (auto& g : ghosts)
    {
        float dist = glm::distance(glm::vec2(pacX, pacZ), glm::vec2(g.x, g.z));

        if (dist < 0.4f) // Raza de coliziune pentru pacman si fantoma
        {
            //std::cout << "GAME OVER!" << std::endl;
            //exit(0);
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON)
    {
        mouseDown = (state == GLUT_DOWN);
        lastMouseX = x;
    }
}

void motion(int x, int y)
{
    if (mouseDown && lastMouseX >= 0)
    {
        float delta = (x - lastMouseX) * 0.01f;
        cameraOrbit += delta;
        lastMouseX = x;
    }
}

std::string textFileRead(char* fn)
{
    std::ifstream ifile(fn);
    std::string filetext;
    while (ifile.good())
    {
        std::string line;
        std::getline(ifile, line);
        filetext.append(line + "\n");
    }
    return filetext;
}

// Construire segmente de pereti
// Greedy meshing
void buildWallSegments()
{
    wallSegments.clear();
    bool visited[MAZE_HEIGHT][MAZE_WIDTH] = { false };

    for (int r = 0; r < MAZE_HEIGHT; r++)
    {
        for (int c = 0; c < MAZE_WIDTH; c++)
        {
            // Cautam un perete neprocesat
            if (maze[r][c] == 1 && !visited[r][c])
            {
                int w = 0;
                // Ne intindem pe orizontala cat putem
                while (c + w < MAZE_WIDTH && maze[r][c + w] == 1 && !visited[r][c + w])
                {
                    w++;
                }

                int d = 0;
                // Daca nu merge pe orizontala, incercam pe verticala
                if (w == 1)
                {
                    while (r + d < MAZE_HEIGHT && maze[r + d][c] == 1 && !visited[r + d][c])
                    {
                        d++;
                    }
                }
                else {
                    d = 1;
                }

                // Marcam segmentul ca vizitat
                for (int i = 0; i < d; i++)
                {
                    for (int j = 0; j < w; j++)
                    {
                        visited[r + i][c + j] = true;
                    }
                }

                WallSegment seg;

                seg.x = (float)c;
                seg.z = (float)r;

                seg.width = (float)w;
                seg.depth = (float)d;

                wallSegments.push_back(seg);
            }
        }
    }
}

void addVertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) 
{
    Vertex vert;
    vert.pos = glm::vec3(x, y, z);
    vert.texCoord = glm::vec2(u, v);
    vert.normal = glm::vec3(nx, ny, nz);
    allWallVertices.push_back(vert);
}

void buildStaticWallBuffer() 
{
    allWallVertices.clear();
    float wallH = 1.0f;

    for (const WallSegment& seg : wallSegments) 
    {
		// Calculam coordonatele celor 8 colturile ale cubului de perete
        float x0 = seg.x - 0.5f;
        float x1 = seg.x + seg.width - 0.5f;
        float z0 = seg.z - 0.5f;
        float z1 = seg.z + seg.depth - 0.5f;
        
        float y0 = -0.5f;
        float y1 = y0 + wallH;

        // Texturam fetele
        float uW = seg.width;
        float uD = seg.depth;
        float vH = wallH;

		// Fata dinspre camera (Z+)
        addVertex(x0, y0, z1, 0.0f, 0.0f, 0, 0, 1); addVertex(x1, y0, z1, uW, 0.0f, 0, 0, 1); addVertex(x1, y1, z1, uW, vH, 0, 0, 1);
        addVertex(x0, y0, z1, 0.0f, 0.0f, 0, 0, 1); addVertex(x1, y1, z1, uW, vH, 0, 0, 1); addVertex(x0, y1, z1, 0.0f, vH, 0, 0, 1);

		// Fata din spate (Z-)
        addVertex(x1, y0, z0, 0.0f, 0.0f, 0, 0, -1); addVertex(x0, y0, z0, uW, 0.0f, 0, 0, -1); addVertex(x0, y1, z0, uW, vH, 0, 0, -1);
        addVertex(x1, y0, z0, 0.0f, 0.0f, 0, 0, -1); addVertex(x0, y1, z0, uW, vH, 0, 0, -1); addVertex(x1, y1, z0, 0.0f, vH, 0, 0, -1);

		// Fata din stanga (X-)
        addVertex(x0, y0, z0, 0.0f, 0.0f, -1, 0, 0); addVertex(x0, y0, z1, uD, 0.0f, -1, 0, 0); addVertex(x0, y1, z1, uD, vH, -1, 0, 0);
        addVertex(x0, y0, z0, 0.0f, 0.0f, -1, 0, 0); addVertex(x0, y1, z1, uD, vH, -1, 0, 0); addVertex(x0, y1, z0, 0.0f, vH, -1, 0, 0);

		// Fata din dreapta (X+)
        addVertex(x1, y0, z1, 0.0f, 0.0f, 1, 0, 0); addVertex(x1, y0, z0, uD, 0.0f, 1, 0, 0); addVertex(x1, y1, z0, uD, vH, 1, 0, 0);
        addVertex(x1, y0, z1, 0.0f, 0.0f, 1, 0, 0); addVertex(x1, y1, z0, uD, vH, 1, 0, 0); addVertex(x1, y1, z1, 0.0f, vH, 1, 0, 0);

		// Fata de sus (Y+)
        addVertex(x0, y1, z1, 0.0f, uD, 0, 1, 0); addVertex(x1, y1, z1, uW, uD, 0, 1, 0); addVertex(x1, y1, z0, uW, 0.0f, 0, 1, 0);
        addVertex(x0, y1, z1, 0.0f, uD, 0, 1, 0); addVertex(x1, y1, z0, uW, 0.0f, 0, 1, 0); addVertex(x0, y1, z0, 0.0f, 0.0f, 0, 1, 0);

		// Fata de jos (Y-)
        addVertex(x0, y0, z0, 0.0f, uD, 0, -1, 0); addVertex(x1, y0, z0, uW, uD, 0, -1, 0); addVertex(x1, y0, z1, uW, 0.0f, 0, -1, 0);
        addVertex(x0, y0, z0, 0.0f, uD, 0, -1, 0); addVertex(x1, y0, z1, uW, 0.0f, 0, -1, 0); addVertex(x0, y0, z1, 0.0f, 0.0f, 0, -1, 0);
    }

	// Dupa ce am generat toti vertexii pentru toate segmentele, ii incarcam o data in buffer
    glBindVertexArray(wallBoxVao);
    glBindBuffer(GL_ARRAY_BUFFER, wallBoxVbo);
    glBufferData(GL_ARRAY_BUFFER, allWallVertices.size() * sizeof(Vertex), allWallVertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

void initPellets()
{
    for (int r = 0; r < MAZE_HEIGHT; r++)
    {
        for (int c = 0; c < MAZE_WIDTH; c++)
        {
            if (maze[r][c] == 0 && (rand() % 100 < 50))
            {
                pellets[r][c] = true;
            }
            else
            {
                pellets[r][c] = false;
            }
        }
    }
    pellets[1][1] = false;
}

void loadWallTexture()
{
    glGenTextures(1, &wallTexture);
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("Plastic008_1K-PNG_Color.png", &width, &height, &nrChannels, 0);

    if (data) 
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
        printf("Wall texture loaded: %dx%d, %d channels\n", width, height, nrChannels);
    }
    else 
    {
        printf("ERROR: Failed to load texture!\n");
    }

    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Se apeleaza de 2 ori: o data pentru a desena scena normala, si o data pentru a desena in depth map
void renderScene(const ShaderUniforms& u)
{
    if (u.useLightLoc != -1) glUniform1i(u.useLightLoc, 1);
    if (u.useTexLoc != -1) glUniform1i(u.useTexLoc, 1);

    modelMatrix = glm::mat4(1.0f);

    if (u.mvpLoc != -1) glUniformMatrix4fv(u.mvpLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
    if (u.modelLoc != -1) glUniformMatrix4fv(u.modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (u.normalLoc != -1) glUniformMatrix4fv(u.normalLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

	// Peretii
    glBindVertexArray(wallBoxVao);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)allWallVertices.size());

    // Podeaua
    if (u.useTexLoc != -1) glUniform1i(u.useTexLoc, 0);
    if (u.colorLoc != -1) glUniform3f(u.colorLoc, 0.08f, 0.08f, 0.18f);

    glBindVertexArray(cubeVao);
    float floorCenterX = (MAZE_WIDTH - 1) / 2.0f;
    float floorCenterZ = (MAZE_HEIGHT - 1) / 2.0f;

    modelMatrix = glm::translate(glm::vec3(floorCenterX, -0.5f, floorCenterZ));
    modelMatrix = glm::scale(modelMatrix, glm::vec3((float)MAZE_WIDTH, 0.08f, (float)MAZE_HEIGHT));

    if (u.mvpLoc != -1) glUniformMatrix4fv(u.mvpLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
    if (u.modelLoc != -1) glUniformMatrix4fv(u.modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (u.normalLoc != -1) glUniformMatrix4fv(u.normalLoc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelMatrix))));

    glDrawElements(GL_TRIANGLES, cubeElementCount, GL_UNSIGNED_INT, NULL);

    // Pellet
    if (u.useLightLoc != -1) glUniform1i(u.useLightLoc, 1);
    if (u.colorLoc != -1) glUniform3f(u.colorLoc, 1.0f, 0.72f, 0.67f);

    glBindVertexArray(sphereVao);
    for (int r = 0; r < MAZE_HEIGHT; r++) 
    {
        for (int c = 0; c < MAZE_WIDTH; c++) 
        {
            if (pellets[r][c]) {
                modelMatrix = glm::translate(glm::vec3((float)c, -0.3f, (float)r));
                modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f));

                if (u.mvpLoc != -1) glUniformMatrix4fv(u.mvpLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
                if (u.modelLoc != -1) glUniformMatrix4fv(u.modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
                if (u.normalLoc != -1) glUniformMatrix4fv(u.normalLoc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelMatrix))));

                glDrawElements(GL_TRIANGLES, sphereElementCount, GL_UNSIGNED_INT, NULL);
            }
        }
    }

    // Fantome
    glBindVertexArray(ghostVao);
    for (const auto& g : ghosts) 
    {
        if (u.colorLoc != -1) glUniform3fv(u.colorLoc, 1, glm::value_ptr(g.color));

        modelMatrix = glm::translate(glm::vec3(g.x, -0.1f, g.z));
        modelMatrix = glm::rotate(modelMatrix, g.currentAngle, glm::vec3(0, 1, 0));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));

        if (u.mvpLoc != -1) glUniformMatrix4fv(u.mvpLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
        if (u.modelLoc != -1) glUniformMatrix4fv(u.modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        if (u.normalLoc != -1) glUniformMatrix4fv(u.normalLoc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelMatrix))));

        glDrawArrays(GL_TRIANGLES, 0, ghostVertices.size());
    }

    // Pacman
    if (u.colorLoc != -1) glUniform3f(u.colorLoc, 1.0f, 0.9f, 0.0f);

    glBindVertexArray(sphereVao);
    modelMatrix = glm::translate(glm::vec3(pacX, 0.0f, pacZ));
    modelMatrix = glm::rotate(modelMatrix, currentAngle, glm::vec3(0, 1, 0));
    modelMatrix = glm::scale(modelMatrix, glm::vec3(0.35f, 0.35f, 0.35f));

    if (u.mvpLoc != -1) glUniformMatrix4fv(u.mvpLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix * modelMatrix));
    if (u.modelLoc != -1) glUniformMatrix4fv(u.modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (u.normalLoc != -1) glUniformMatrix4fv(u.normalLoc, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(modelMatrix))));

    glDrawElements(GL_TRIANGLES, sphereElementCount, GL_UNSIGNED_INT, NULL);
}

void display()
{
    // Logica jocului - actualizare pozitie, verificare coliziuni, etc.
    //
    // 

    float currentFrameTime = glutGet(GLUT_ELAPSED_TIME) / 1'000.0f; // s
    float deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;

	// Minimizarea deltaTime pentru a evita probleme de fizica)
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // Logica pentru lerping pentru miscarea lui Pacman
    float interpSpeed = 10.0f * deltaTime;
    pacX += (targetX - pacX) * interpSpeed;
    pacZ += (targetZ - pacZ) * interpSpeed;

    // Consumare pellet
    int currentGridX = (int)(pacX + 0.5f);
    int currentGridZ = (int)(pacZ + 0.5f);

    // Verificam daca pozitia curenta a lui Pacman corespunde unui pellet si,
    // daca da, il consumam (il dezactivam)
    if (currentGridX >= 0 && currentGridX < MAZE_WIDTH &&
        currentGridZ >= 0 && currentGridZ < MAZE_HEIGHT)
    {
        if (pellets[currentGridZ][currentGridX])
        {
            pellets[currentGridZ][currentGridX] = false;
        }
    }

    // Logica teleportarii: daca Pacman depaseste marginile labirintului,
    // il aducem pe partea cealalta
    if (pacX < -0.5f)
    {
        pacX += MAZE_WIDTH;
        targetX += MAZE_WIDTH;
    }
    else if (pacX > MAZE_WIDTH - 0.5f)
    {
        pacX -= MAZE_WIDTH;
        targetX -= MAZE_WIDTH;
    }

    // Logica pentru rotirea lui pacman + camera in directia miscarii
    // 1D Shortest-Path Lerp
    float rotationFactor = 10.0f * deltaTime;

    float angleDiff = targetAngle - currentAngle;
    while (angleDiff > PI) angleDiff -= 2.0f * PI;
    while (angleDiff < -PI) angleDiff += 2.0f * PI;
    currentAngle += angleDiff * rotationFactor;

    float camDiff = cameraTarget - cameraAngle;
    while (camDiff > PI) camDiff -= 2.0f * PI;
    while (camDiff < -PI) camDiff += 2.0f * PI;
    cameraAngle += camDiff * rotationFactor;

    // Camera in spatele lui pacman
    float distanceBehind = 5.0f;
    float heightAbove = 3.5f;
    float finalCamAngle = cameraAngle + cameraOrbit;

    // Calculam pozitia camerei folosind un offset polar fata de pozitia lui pacman
    float camX = pacX + sin(finalCamAngle) * distanceBehind;
    float camZ = pacZ + cos(finalCamAngle) * distanceBehind;
    viewPos = glm::vec3(camX, heightAbove, camZ);
    viewMatrix = glm::lookAt(viewPos, glm::vec3(pacX, 0.3f, pacZ), glm::vec3(0, 1, 0));

    updateGhosts(deltaTime);
    checkGameOver();

    // Shadow map setup
    // 
    // 

    // Pozitia luminii
    float size = 25.0f;
    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, 0.1f, 60.0f);

    // Pentru a acoperi intregul labirint, pozitionam lumina deasupra centrului acestuia
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(MAZE_WIDTH / 2.0f, 0.0f, MAZE_HEIGHT / 2.0f), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // Randare in depth map
    glUseProgram(depth_shader_programme);
    glUniformMatrix4fv(depthUniforms.lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Desenam toate obiectele pentru depth buffer
    renderScene(depthUniforms);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Randam scena finala
    glUseProgram(shader_programme);

    // Resetam viewport-ul la dimensiunea ferestrei pentru a desena scena finala
    glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    glUniform3fv(lightUniforms.viewPosLoc, 1, glm::value_ptr(viewPos));

    glUniformMatrix4fv(lightUniforms.lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // Textura peretilor 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);
    //glUniform1i(textureLoc, 0);

    // Depth map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    // Desenam toate obiectele normal
    renderScene(lightUniforms);

    glutSwapBuffers();
    glutPostRedisplay();
}

void init()
{
    glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glewInit();

    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fantomele
    bool res = loadOBJ("obj/Ghost.obj", ghostVertices, ghostUVs, ghostNormals);

    glGenVertexArrays(1, &ghostVao);
    glBindVertexArray(ghostVao);

    // Pozitiile vertexilor
    glGenBuffers(1, &ghostVboPos);
    glBindBuffer(GL_ARRAY_BUFFER, ghostVboPos);
    glBufferData(GL_ARRAY_BUFFER, ghostVertices.size() * sizeof(glm::vec3), &ghostVertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Normalele vertexilor
    glGenBuffers(1, &ghostVboNorm);
    glBindBuffer(GL_ARRAY_BUFFER, ghostVboNorm);
    glBufferData(GL_ARRAY_BUFFER, ghostNormals.size() * sizeof(glm::vec3), &ghostNormals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

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

    glGenVertexArrays(1, &wallBoxVao);
    glGenBuffers(1, &wallBoxVbo);

    buildWallSegments();
    buildStaticWallBuffer();

    loadWallTexture();
    printf("Wall draw calls reduced from %d worst case to %d total\n", MAZE_HEIGHT * MAZE_WIDTH, (int)wallSegments.size());

    initPellets();
    initGhosts();

    std::string vsLightText = textFileRead("pixel_light.vert");
    std::string fsLightText = textFileRead("pixel_light.frag");
    std::string vsDepthText = textFileRead("depth.vert");
    std::string fsDepthText = textFileRead("depth.frag");

    const char* lightVertex_shader = vsLightText.c_str();
    const char* lightFragment_shader = fsLightText.c_str();
    const char* depthVertex_shader = vsDepthText.c_str();
    const char* depthFragment_shader = fsDepthText.c_str();

    GLuint vsLight = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsLight, 1, &lightVertex_shader, NULL);
    glCompileShader(vsLight);
    GLuint vsDepth = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsDepth, 1, &depthVertex_shader, NULL);
    glCompileShader(vsDepth);

    GLuint fsLight = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsLight, 1, &lightFragment_shader, NULL);
    glCompileShader(fsLight);
    GLuint fsDepth = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsDepth, 1, &depthFragment_shader, NULL);
    glCompileShader(fsDepth);

    shader_programme = glCreateProgram();
    depth_shader_programme = glCreateProgram();

    glAttachShader(shader_programme, fsLight);
    glAttachShader(depth_shader_programme, fsDepth);
    glAttachShader(shader_programme, vsLight);
    glAttachShader(depth_shader_programme, vsDepth);

    glLinkProgram(shader_programme);
    glLinkProgram(depth_shader_programme);

	// Cache pentru shaderul de iluminare
    lightUniforms.mvpLoc = glGetUniformLocation(shader_programme, "mvpMatrix");
    lightUniforms.modelLoc = glGetUniformLocation(shader_programme, "modelMatrix");
    lightUniforms.normalLoc = glGetUniformLocation(shader_programme, "normalMatrix");
    lightUniforms.colorLoc = glGetUniformLocation(shader_programme, "objectColor");
    lightUniforms.useLightLoc = glGetUniformLocation(shader_programme, "useLighting");
    lightUniforms.useTexLoc = glGetUniformLocation(shader_programme, "useTexture");

    lightUniforms.lightSpaceLoc = glGetUniformLocation(shader_programme, "lightSpaceMatrix");
    lightUniforms.lightPosLoc = glGetUniformLocation(shader_programme, "lightPos");
    lightUniforms.viewPosLoc = glGetUniformLocation(shader_programme, "viewPos");
    lightUniforms.textureLoc = glGetUniformLocation(shader_programme, "wallTexture");
    lightUniforms.shadowMapLoc = glGetUniformLocation(shader_programme, "shadowMap");

	// Cache pentru shaderul de depth
    depthUniforms.mvpLoc = glGetUniformLocation(depth_shader_programme, "mvpMatrix");
    depthUniforms.modelLoc = glGetUniformLocation(depth_shader_programme, "modelMatrix");
    depthUniforms.normalLoc = glGetUniformLocation(depth_shader_programme, "normalMatrix");
    depthUniforms.colorLoc = glGetUniformLocation(depth_shader_programme, "objectColor");
    depthUniforms.useLightLoc = glGetUniformLocation(depth_shader_programme, "useLighting");
    depthUniforms.useTexLoc = glGetUniformLocation(depth_shader_programme, "useTexture");

    depthUniforms.lightSpaceLoc = glGetUniformLocation(depth_shader_programme, "lightSpaceMatrix");

    glUseProgram(shader_programme);
    glUniform1i(lightUniforms.textureLoc, 0);
    glUniform1i(lightUniforms.shadowMapLoc, 1);
    glUniform3fv(lightUniforms.lightPosLoc, 1, glm::value_ptr(lightPos));
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    projectionMatrix = glm::perspective(PI / 4, (float)w / h, 0.1f, 100.0f);
}

void keyboard(unsigned char key, int x, int y) {
    float nextX = targetX;
    float nextZ = targetZ;

    switch (key) {
        // Pentru a si d nu se misca lateral, ci sa se intoarca la 90 de grade stanga/dreapta,
        // nu este nevoie de determinarea coliziunii
    case 'a':
        targetAngle += PI / 2.0f;
        cameraTarget = targetAngle + PI;
        glutPostRedisplay();
        return;
    case 'd':
        targetAngle -= PI / 2.0f;
        cameraTarget = targetAngle + PI;
        glutPostRedisplay();
        return;
    case 'w':
        nextX += sin(targetAngle);
        nextZ += cos(targetAngle);
        break;
    case 's':
        nextX -= sin(targetAngle);
        nextZ -= cos(targetAngle);
        break;
    case 27:
        exit(0); // ESC key
    }

    int gridX = (int)(nextX + 0.5f);
    int gridZ = (int)(nextZ + 0.5f);

    // Logica de teleportare: daca Pacman incearca sa iasa pe latura stanga sau dreapta 
    // a labirintului prin tunel, il aducem pe partea cealalta
    if (gridZ == 7 && (gridX < 0 || gridX >= MAZE_WIDTH))
    {
        targetX = nextX;
        targetZ = nextZ;
    }
    // Daca nu e tunel, verificam coliziunea normala cu peretii
    else if (gridX >= 0 && gridX < MAZE_WIDTH && gridZ >= 0 && gridZ < MAZE_HEIGHT)
    {
        if (maze[gridZ][gridX] != 1)
        {
            targetX = nextX;
            targetZ = nextZ;
        }
    }

    glutPostRedisplay();
}

void specialKeyboard(int key, int x, int y)
{
    unsigned char mapped = 0;
    switch (key) {
    case GLUT_KEY_UP:
        mapped = 'w';
        break;
    case GLUT_KEY_DOWN:
        mapped = 's';
        break;
    case GLUT_KEY_LEFT:
        mapped = 'a';
        break;
    case GLUT_KEY_RIGHT:
        mapped = 'd';
        break;
    }

    if (mapped)
    {
        keyboard(mapped, x, y);
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE); // Folosim double buffering pentru a preveni flickering-ul
    glutInitWindowPosition(200, 200);
    glutInitWindowSize(1000, 800);
    glutCreateWindow("Pac-Man");

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