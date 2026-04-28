#pragma once

#include <glm/vec3.hpp>
#include <vector>

class CubeMesh {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::ivec3> triangles;

    CubeMesh() {
        // 8 Vertices of a unit cube (from -0.5 to 0.5)
        vertices = {
            {-0.5f, -0.5f,  0.5f}, {0.5f, -0.5f,  0.5f},
            {0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
            {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f},
            {0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}
        };

        // 12 Triangles (indices)
        triangles = {
            {0, 1, 2}, {2, 3, 0}, // Front
            {1, 5, 6}, {6, 2, 1}, // Right
            {7, 6, 5}, {5, 4, 7}, // Back
            {4, 0, 3}, {3, 7, 4}, // Left
            {4, 5, 1}, {1, 0, 4}, // Bottom
            {3, 2, 6}, {6, 7, 3}  // Top
        };
    }
};