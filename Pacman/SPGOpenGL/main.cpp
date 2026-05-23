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

glm::vec3 lightPos(10.0f, 20.0f, 15.0f); // Pozitia sursei de lumina
glm::vec3 viewPos(5.0f, 18.0f, 12.0f); // Pozitia initiala a camerei

// Pentru shadow mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; // Rezolutia
GLuint depth_shader_programme; // Avem nevoie de un shader separat pentru a desena in depth map

// Pentru halo
GLuint haloShaderProg;
GLuint haloVao, haloVbo;
GLint haloProjLoc, haloViewLoc, haloCenterLoc, haloSizeLoc, haloColorLoc;

// Cubemap
GLuint skyboxShaderProg;
GLuint skyboxVAO, skyboxVBO;
GLuint cubemapTexture;
GLuint wallNormalMap;

bool enableNormalMapping = true;
bool enableGlow = true;
bool enableShadows = true;

GLint skyboxProjLoc, skyboxViewLoc, skyboxTexLoc;

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

bool isGameOver = false;
bool isGameWon = false;

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
    glm::vec3 tangent;
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

std::vector<Ghost> ghosts;

struct ShaderUniforms {
    GLint mvpLoc = -1;
    GLint modelLoc = -1;
    GLint normalLoc = -1;
    GLint colorLoc = -1;
    GLint useLightLoc = -1;
    GLint useTexLoc = -1;

    GLint useNormalMappingLoc = -1;
    GLint useShadowsLoc = -1;

    GLint lightSpaceLoc = -1;
    GLint lightPosLoc = -1;
    GLint viewPosLoc = -1;
    GLint textureLoc = -1;
    GLint shadowMapLoc = -1;
    GLint normalMapLoc = -1;
};

ShaderUniforms lightUniforms;   // pentru shader_programme
ShaderUniforms depthUniforms;   // pentru depth_shader_programme

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

void renderText2D(float x, float y, void* font, const std::string& text, glm::vec3 color)
{
    // Oprim testul pentru a afisa textul deasupra 
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0); // Unbind pentru a elimina orice shader activ

    // Activam blending pentru a face fundalul semitransparent
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);

    glBegin(GL_QUADS);
    glVertex2f(-1.0f, -1.0f);
    glVertex2f(1.0f, -1.0f);
    glVertex2f(1.0f, 1.0f);
    glVertex2f(-1.0f, 1.0f);
    glEnd();

    // Dupa desenarea fundalului, dezactivam blending-ul pentru a nu afecta restul scenei
    glDisable(GL_BLEND);

    glColor3f(color.r, color.g, color.b);

    // Pozititonam textul in coordonate de ecran
    glRasterPos2f(x, y);

    for (char c : text)
    {
        glutBitmapCharacter(font, c);
    }

    // Reactivam testul de adancime pentru restul scenei
    glEnable(GL_DEPTH_TEST);
}

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

void loadNormalMap()
{
    glGenTextures(1, &wallNormalMap);
    glBindTexture(GL_TEXTURE_2D, wallNormalMap);
    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("img/WallNormal.png", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

void addVertex(
    float x, float y, float z,
    float u, float v,
    float nx, float ny, float nz,
    float tx, float ty, float tz
)
{
    Vertex vert;

    vert.pos = glm::vec3(x, y, z);
    vert.texCoord = glm::vec2(u, v);
    vert.normal = glm::vec3(nx, ny, nz);
    vert.tangent = glm::vec3(tx, ty, tz);

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
        addVertex(x0, y0, z1, 0.0f, 0.0f, 0, 0, 1, 1, 0, 0);
        addVertex(x1, y0, z1, uW, 0.0f, 0, 0, 1, 1, 0, 0);
        addVertex(x1, y1, z1, uW, vH, 0, 0, 1, 1, 0, 0);

        addVertex(x0, y0, z1, 0.0f, 0.0f, 0, 0, 1, 1, 0, 0);
        addVertex(x1, y1, z1, uW, vH, 0, 0, 1, 1, 0, 0);
        addVertex(x0, y1, z1, 0.0f, vH, 0, 0, 1, 1, 0, 0);

        // Fata din spate (Z-)
        addVertex(x1, y0, z0, 0.0f, 0.0f, 0, 0, -1, -1, 0, 0);
        addVertex(x0, y0, z0, uW, 0.0f, 0, 0, -1, -1, 0, 0);
        addVertex(x0, y1, z0, uW, vH, 0, 0, -1, -1, 0, 0);

        addVertex(x1, y0, z0, 0.0f, 0.0f, 0, 0, -1, -1, 0, 0);
        addVertex(x0, y1, z0, uW, vH, 0, 0, -1, -1, 0, 0);
        addVertex(x1, y1, z0, 0.0f, vH, 0, 0, -1, -1, 0, 0);

        // Fata din stanga (X-)
        addVertex(x0, y0, z0, 0.0f, 0.0f, -1, 0, 0, 0, 0, 1);
        addVertex(x0, y0, z1, uD, 0.0f, -1, 0, 0, 0, 0, 1);
        addVertex(x0, y1, z1, uD, vH, -1, 0, 0, 0, 0, 1);

        addVertex(x0, y0, z0, 0.0f, 0.0f, -1, 0, 0, 0, 0, 1);
        addVertex(x0, y1, z1, uD, vH, -1, 0, 0, 0, 0, 1);
        addVertex(x0, y1, z0, 0.0f, vH, -1, 0, 0, 0, 0, 1);

        // Fata din dreapta (X+)
        addVertex(x1, y0, z1, 0.0f, 0.0f, 1, 0, 0, 0, 0, -1);
        addVertex(x1, y0, z0, uD, 0.0f, 1, 0, 0, 0, 0, -1);
        addVertex(x1, y1, z0, uD, vH, 1, 0, 0, 0, 0, -1);

        addVertex(x1, y0, z1, 0.0f, 0.0f, 1, 0, 0, 0, 0, -1);
        addVertex(x1, y1, z0, uD, vH, 1, 0, 0, 0, 0, -1);
        addVertex(x1, y1, z1, 0.0f, vH, 1, 0, 0, 0, 0, -1);

        // Fata de sus (Y+)
        addVertex(x0, y1, z1, 0.0f, uD, 0, 1, 0, 1, 0, 0);
        addVertex(x1, y1, z1, uW, uD, 0, 1, 0, 1, 0, 0);
        addVertex(x1, y1, z0, uW, 0.0f, 0, 1, 0, 1, 0, 0);

        addVertex(x0, y1, z1, 0.0f, uD, 0, 1, 0, 1, 0, 0);
        addVertex(x1, y1, z0, uW, 0.0f, 0, 1, 0, 1, 0, 0);
        addVertex(x0, y1, z0, 0.0f, 0.0f, 0, 1, 0, 1, 0, 0);

        // Fata de jos (Y-)
        addVertex(x0, y0, z0, 0.0f, uD, 0, -1, 0, 1, 0, 0);
        addVertex(x1, y0, z0, uW, uD, 0, -1, 0, 1, 0, 0);
        addVertex(x1, y0, z1, uW, 0.0f, 0, -1, 0, 1, 0, 0);

        addVertex(x0, y0, z0, 0.0f, uD, 0, -1, 0, 1, 0, 0);
        addVertex(x1, y0, z1, uW, 0.0f, 0, -1, 0, 1, 0, 0);
        addVertex(x0, y0, z1, 0.0f, 0.0f, 0, -1, 0, 1, 0, 0);
    }

    // Dupa ce am generat toti vertexii pentru toate segmentele, ii incarcam o data in buffer
    glBindVertexArray(wallBoxVao);
    glBindBuffer(GL_ARRAY_BUFFER, wallBoxVbo);
    glBufferData(GL_ARRAY_BUFFER, allWallVertices.size() * sizeof(Vertex), allWallVertices.data(), GL_STATIC_DRAW);

    // Pozitiile
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    // UV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

    // Normalele
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

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
    unsigned char* data = stbi_load("img/Wall.jpg", &width, &height, &nrChannels, 0);

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

GLuint loadCubemap(std::vector<std::string> faces)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;

    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

        if (data)
        {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void restartGame()
{
    pacX = 9.0f;
    pacZ = 10.0f;
    targetX = 9.0f;
    targetZ = 10.0f;

    currentAngle = PI;
    targetAngle = PI;
    cameraAngle = PI;
    cameraTarget = PI;
    cameraOrbit = 0.0f;

    initPellets();
    initGhosts();

    isGameOver = false;
    isGameWon = false;
    printf("Game restarted!\n");
}

void checkGameOver()
{
    for (auto& g : ghosts)
    {
        if (isGameOver)
        {
            return;
        }

        float dist = glm::distance(glm::vec2(pacX, pacZ), glm::vec2(g.x, g.z));

        if (dist < 0.4f) // Raza de coliziune pentru pacman si fantoma
        {
            isGameOver = true;
            break;
        }
    }
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
    if (u.colorLoc != -1) glUniform3f(u.colorLoc, 1.0f, 1.0f, 1.0f);

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

    if (!isGameOver && !isGameWon)
    {
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

                bool pelletsLeft = false;
                for (int r = 0; r < MAZE_HEIGHT; r++) 
                {
                    for (int c = 0; c < MAZE_WIDTH; c++) 
                    {
                        if (pellets[r][c]) 
                        {
                            pelletsLeft = true;
                            break;
                        }
                    }
                    if (pelletsLeft) break;
                }

				// Daca nu mai avem niciun pellet, jucatorul a castigat
                if (!pelletsLeft) 
                {
                    isGameWon = true;
                }
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
    }

    // Camera in spatele lui pacman
    float distanceBehind = 5.0f;
    float heightAbove = 3.5f;
    float finalCamAngle = cameraAngle + cameraOrbit;

    // Calculam pozitia camerei folosind un offset polar fata de pozitia lui pacman
    float camX = pacX + sin(finalCamAngle) * distanceBehind;
    float camZ = pacZ + cos(finalCamAngle) * distanceBehind;
    viewPos = glm::vec3(camX, heightAbove, camZ);
    viewMatrix = glm::lookAt(viewPos, glm::vec3(pacX, 0.3f, pacZ), glm::vec3(0, 1, 0));

    if (!isGameOver && !isGameWon)
    {
        updateGhosts(deltaTime);
        checkGameOver();
    }

    // Shadow map setup
    // 
    // 

    // Pozitia luminii
    float size = 25.0f;
    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, 0.1f, 60.0f);

    // Pentru a acoperi intregul labirint, pozitionam lumina deasupra centrului acestuia
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(MAZE_WIDTH / 2.0f, 0.0f, MAZE_HEIGHT / 2.0f), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    if (enableShadows)
    {
        // Randare in depth map
        glUseProgram(depth_shader_programme);
        glUniformMatrix4fv(depthUniforms.lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Desenam toate obiectele pentru depth buffer
        renderScene(depthUniforms);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Randam scena finala
    glUseProgram(shader_programme);

    // Resetam viewport-ul la dimensiunea ferestrei pentru a desena scena finala
    glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUniform3fv(lightUniforms.viewPosLoc, 1, glm::value_ptr(viewPos));

    glUniformMatrix4fv(lightUniforms.lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    if (lightUniforms.useNormalMappingLoc != -1)
    {
        glUniform1i(lightUniforms.useNormalMappingLoc, enableNormalMapping ? 1 : 0);
    }

    if (lightUniforms.useShadowsLoc != -1)
    {
        glUniform1i(lightUniforms.useShadowsLoc, enableShadows ? 1 : 0);
    }
    
    // Textura peretilor 
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, wallTexture);

    // Normal map pentru pereti
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, wallNormalMap);

    // Depth map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);

    // Desenam toate obiectele normal
    renderScene(lightUniforms);

    // Cubemap pentru skybox
    // Desenam skybox-ul ultimul, in spate la toate
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxShaderProg);

    glUniformMatrix4fv(skyboxProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(skyboxViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glBindVertexArray(skyboxVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Resetam functia de adancime
    glDepthFunc(GL_LESS);

    if (enableGlow)
    {
        glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Adaugam culorile pentru un efect de glow mai puternic

        glUseProgram(haloShaderProg);
        glUniformMatrix4fv(haloProjLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(haloViewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));

        glUniform3f(haloColorLoc, 1.0f, 1.0f, 1.0f);
        glUniform1f(haloSizeLoc, 0.21f);

        glBindVertexArray(haloVao);

		// Pentru fiecare pellet activ, desenam halo
        for (int r = 0; r < MAZE_HEIGHT; r++)
        {
            for (int c = 0; c < MAZE_WIDTH; c++)
            {
                if (pellets[r][c]) 
                {
                    glm::vec3 center = glm::vec3((float)c, -0.3f, (float)r);
                    glUniform3fv(haloCenterLoc, 1, glm::value_ptr(center));
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }
        glBindVertexArray(0);

		// Resetam starea pentru a nu afecta restul obiectelor
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // Afisare text de Game Over
    if (isGameOver)
    {
        renderText2D(-0.15f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24, "GAME OVER", glm::vec3(1.0f, 0.0f, 0.0f));
        renderText2D(-0.28f, -0.05f, GLUT_BITMAP_HELVETICA_18, "Press 'R' to restart the game!", glm::vec3(1.0f, 1.0f, 1.0f));
    }
    else if (isGameWon)
    {
        // Bright green text for the win
        renderText2D(-0.13f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24, "YOU WIN!", glm::vec3(0.0f, 1.0f, 0.0f));
        renderText2D(-0.24f, -0.05f, GLUT_BITMAP_HELVETICA_18, "Press 'R' to play again!", glm::vec3(1.0f, 1.0f, 1.0f));
    }

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
    loadNormalMap();

    printf("Wall draw calls reduced from %d worst case to %d total\n", MAZE_HEIGHT * MAZE_WIDTH, (int)wallSegments.size());

	// Patrat pentru halo
    float squareVertices[] = 
    {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f, -1.0f,
        1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &haloVao);
    glGenBuffers(1, &haloVbo);
    glBindVertexArray(haloVao);
    glBindBuffer(GL_ARRAY_BUFFER, haloVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(squareVertices), &squareVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::string vsHaloText = textFileRead("halo.vert");
    std::string fsHaloText = textFileRead("halo.frag");
    const char* haloVertex_shader = vsHaloText.c_str();
    const char* haloFragment_shader = fsHaloText.c_str();

    GLuint vsHalo = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsHalo, 1, &haloVertex_shader, NULL);
    glCompileShader(vsHalo);

    GLuint fsHalo = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsHalo, 1, &haloFragment_shader, NULL);
    glCompileShader(fsHalo);

    haloShaderProg = glCreateProgram();
    glAttachShader(haloShaderProg, vsHalo);
    glAttachShader(haloShaderProg, fsHalo);
    glLinkProgram(haloShaderProg);

	// Cache pentru shaderul de halo
    haloProjLoc = glGetUniformLocation(haloShaderProg, "projection");
    haloViewLoc = glGetUniformLocation(haloShaderProg, "view");
    haloCenterLoc = glGetUniformLocation(haloShaderProg, "centerPos");
    haloSizeLoc = glGetUniformLocation(haloShaderProg, "size");
    haloColorLoc = glGetUniformLocation(haloShaderProg, "haloColor");

    restartGame();

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
    lightUniforms.normalMapLoc = glGetUniformLocation(shader_programme, "normalMap");

    lightUniforms.useNormalMappingLoc = glGetUniformLocation(shader_programme, "useNormalMapping");
    lightUniforms.useShadowsLoc = glGetUniformLocation(shader_programme, "useShadows");

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
    glUniform1i(lightUniforms.normalMapLoc, 2);
    glUniform1i(lightUniforms.shadowMapLoc, 1);

    glUniform3fv(lightUniforms.lightPosLoc, 1, glm::value_ptr(lightPos));

    // Initializare skybox
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f
    };

    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces{
        "img/right.png",
        "img/left.png",
        "img/top.png",
        "img/bottom.png",
        "img/front.png",
        "img/back.png"
    };
    cubemapTexture = loadCubemap(faces);

    std::string vsSkyboxText = textFileRead("skybox.vert");
    std::string fsSkyboxText = textFileRead("skybox.frag");
    const char* skyboxVertex_shader = vsSkyboxText.c_str();
    const char* skyboxFragment_shader = fsSkyboxText.c_str();

    GLuint vsSkybox = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsSkybox, 1, &skyboxVertex_shader, NULL);
    glCompileShader(vsSkybox);

    GLuint fsSkybox = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsSkybox, 1, &skyboxFragment_shader, NULL);
    glCompileShader(fsSkybox);

    skyboxShaderProg = glCreateProgram();
    glAttachShader(skyboxShaderProg, vsSkybox);
    glAttachShader(skyboxShaderProg, fsSkybox);
    glLinkProgram(skyboxShaderProg);

    skyboxProjLoc = glGetUniformLocation(skyboxShaderProg, "projection");
    skyboxViewLoc = glGetUniformLocation(skyboxShaderProg, "view");
    skyboxTexLoc = glGetUniformLocation(skyboxShaderProg, "skybox");

    glUseProgram(skyboxShaderProg);
    glUniform1i(skyboxTexLoc, 0);
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    projectionMatrix = glm::perspective(PI / 4, (float)w / h, 0.1f, 100.0f);
}

void keyboard(unsigned char key, int x, int y)
{

    if (key == 'r' || key == 'R')
    {
        restartGame();
        glutPostRedisplay();
        return;
    }

    if (isGameOver || isGameWon)
    {
        return;
    }

    float nextX = targetX;
    float nextZ = targetZ;

    // --- NEW CAMERA-RELATIVE MATH ---
    // 1. Get the camera's actual view angle including the mouse drag
    float finalCamAngle = cameraAngle + cameraOrbit;

    // 2. "Up" on the screen is the direction pointing AWAY from the camera
    float screenUpAngle = finalCamAngle + PI;

    // 3. Snap that angle to the nearest 90 degrees (PI / 2) to keep Pac-Man on the grid
    float step = PI / 2.0f;
    float snappedUp = round(screenUpAngle / step) * step;

    switch (key) 
    {
        // Fullscreen
    case 'f':
    case 'F':
        glutFullScreenToggle();
        printf("Fullscreen mode activated!\n");
        return;

        // Pentru testare
    case 'k':
    case 'K':
        printf("Cheat code activated!\n");
        for (int r = 0; r < MAZE_HEIGHT; r++)
        {
            for (int c = 0; c < MAZE_WIDTH; c++)
            {
                pellets[r][c] = false;
            }
        }

        pellets[9][9] = true;
        pellets[8][9] = true;
        pellets[7][9] = true;
        return;
        
        // Toggle pentru glow
    case 'g':
    case 'G':
        enableGlow = !enableGlow;
        printf("Glow effect: %s\n", enableGlow ? "ON" : "OFF");
        glutPostRedisplay();
        return;

		// Toggle pentru umbre
    case 'm':
    case 'M':
        enableShadows = !enableShadows;
        printf("Shadows: %s\n", enableShadows ? "ON" : "OFF");
        glutPostRedisplay();
        return;

		// Toggle pentru normal mapping
    case 'n':
    case 'N':
        enableNormalMapping = !enableNormalMapping;
        printf("Normal mapping: %s\n", enableNormalMapping ? "ON" : "OFF");
        glutPostRedisplay();
        return;

        // Pentru a si d nu se misca lateral, ci sa se intoarca la 90 de grade stanga/dreapta,
        // nu este nevoie de determinarea coliziunii
    case 'a':
    case 'A':
        targetAngle += PI / 2.0f; // Turn 90 degrees Left
        break;

    case 'd':
    case 'D':
        targetAngle -= PI / 2.0f; // Turn 90 degrees Right
        break;

    case 'w':
    case 'W':
        nextX += sin(targetAngle);
        nextZ += cos(targetAngle); // Walk Forward
        break;

    case 's':
    case 'S':
        nextX -= sin(targetAngle);
        nextZ -= cos(targetAngle); // Walk Backward
        break;

    case 27:
        exit(0); // ESC key
    }

    if (key == 'w' || key == 's' || key == 'a' || key == 'd' ||
        key == 'W' || key == 'S' || key == 'A' || key == 'D')
    {
        // 1. Bake the current mouse drag into the actual camera angle
        // This gives the transition a smooth starting point.
        cameraAngle += cameraOrbit;
        cameraOrbit = 0.0f;

        // 2. Set the target directly behind Pac-Man's new direction
        cameraTarget = targetAngle + PI;

        // 3. Normalize the angles so the camera doesn't spin 360 degrees the long way around!
        while (cameraTarget - cameraAngle > PI) cameraTarget -= 2.0f * PI;
        while (cameraTarget - cameraAngle < -PI) cameraTarget += 2.0f * PI;
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