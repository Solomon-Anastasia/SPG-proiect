#pragma once

#include <glm/vec3.hpp>
#include <vector>

class CubeMesh {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::ivec3> triangles;

    CubeMesh() {
        // Fetile cubului unitate (de la -0.5 la 0.5)
        vertices = {
            {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
            {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
            {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}
        };

        // Indicii
        triangles = {
			{0, 1, 2}, {2, 3, 0}, // Fata
            {1, 5, 6}, {6, 2, 1}, // Dreapta
            {7, 6, 5}, {5, 4, 7}, // Spate
            {4, 0, 3}, {3, 7, 4}, // Stanga
            {4, 5, 1}, {1, 0, 4}, // Jos
            {3, 2, 6}, {6, 7, 3}  // Sus
        };
    }
};