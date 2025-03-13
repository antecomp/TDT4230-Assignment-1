#include <glad/glad.h>
#include <program.hpp>
#include "glutils.h"
#include <vector>
#include "iostream"

template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize) {
    unsigned int bufferID;
    glGenBuffers(1, &bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, bufferID);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
    glEnableVertexAttribArray(id);
    return bufferID;
}


void getTangentBasisForMesh(
    Mesh &mesh, 
    std::vector<glm::vec3> &tangents, 
    std::vector<glm::vec3> &bitangents
) {
    // std::cout << "Starting TBN computation..." << std::endl;
    // std::cout << "Mesh has " << mesh.vertices.size() << " vertices and " 
    //           << mesh.indices.size() / 3 << " triangles." << std::endl;

    // Verify correct size (prevent bad vector ref)
    if (tangents.size() < mesh.vertices.size() || bitangents.size() < mesh.vertices.size()) {
        std::cerr << "❌ ERROR: Tangents/bitangents arrays are too small! "
                  << "Tangents size: " << tangents.size() << ", "
                  << "Bitangents size: " << bitangents.size() << ", "
                  << "Expected: " << mesh.vertices.size() << std::endl;
        return;
    }

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        unsigned int i0 = mesh.indices[i];
        unsigned int i1 = mesh.indices[i + 1];
        unsigned int i2 = mesh.indices[i + 2];

        // Check index validity
        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            std::cerr << "❌ ERROR: Index out of bounds! i0=" << i0 
                      << ", i1=" << i1 << ", i2=" << i2 
                      << ", max index allowed: " << mesh.vertices.size() - 1 << std::endl;
            return;
        }

        glm::vec3 &v0 = mesh.vertices[i0];
        glm::vec3 &v1 = mesh.vertices[i1];
        glm::vec3 &v2 = mesh.vertices[i2];

        if (mesh.textureCoordinates.empty()) {
            std::cerr << "mesh.textureCoordinates is empty. Cannot compute UV deltas." << std::endl;
            return;
        }

        glm::vec2 &uv0 = mesh.textureCoordinates[i0];
        glm::vec2 &uv1 = mesh.textureCoordinates[i1];
        glm::vec2 &uv2 = mesh.textureCoordinates[i2];

        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        glm::vec2 deltaUV1 = uv1 - uv0;
        glm::vec2 deltaUV2 = uv2 - uv0;

        float determinant = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;

        // Check for division by zero
        if (fabs(determinant) < 1e-6) {
            std::cerr << "⚠ WARNING: UV coordinates are degenerate for triangle " 
                      << i / 3 << ", skipping!" << std::endl;
            continue;
        }

        float r = 1.0f / determinant;

        glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

        // Prevent out-of-bounds writes
        if (i0 >= tangents.size() || i1 >= tangents.size() || i2 >= tangents.size()) {
            std::cerr << "❌ ERROR: Attempted to write outside bounds! Skipping triangle." << std::endl;
            return;
        }

        // Accumulate per-vertex
        tangents[i0] += tangent;
        tangents[i1] += tangent;
        tangents[i2] += tangent;
        bitangents[i0] += bitangent;
        bitangents[i1] += bitangent;
        bitangents[i2] += bitangent;
    }
}


unsigned int generateBuffer(Mesh &mesh) {
    // std::cout << "Generating buffer for mesh with " << mesh.vertices.size() 
    //           << " vertices and " << mesh.indices.size() << " indices." << std::endl;

    if (mesh.vertices.empty()) {
        std::cerr << "Error: Mesh has no vertices!" << std::endl;
        return 0;
    }
    if (mesh.indices.empty()) {
        std::cerr << "Error: Mesh has no indices!" << std::endl;
        return 0;
    }

    unsigned int vaoID;
    glGenVertexArrays(1, &vaoID);
    //std::cout << "Generated VAO: " << vaoID << std::endl;
    glBindVertexArray(vaoID);
    
    //std::cout << "Generating vertex attribute 0..." << std::endl;
    generateAttribute(0, 3, mesh.vertices, false);
    
    if (!mesh.normals.empty()) {
        //std::cout << "Generating vertex attribute 1 (normals)..." << std::endl;
        generateAttribute(1, 3, mesh.normals, true);
    }

    if (!mesh.textureCoordinates.empty()) {
        //std::cout << "Generating vertex attribute 2 (texture coords)..." << std::endl;
        generateAttribute(2, 2, mesh.textureCoordinates, false);
    }

    // Tangent/Bitangent (TBN) calculation
    //std::cout << "Calculating TBN basis..." << std::endl;
    std::vector<glm::vec3> tangents(mesh.vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> bitangents(mesh.vertices.size(), glm::vec3(0.0f));

    // CURRENTLY BROKEN WITH LOADER (SEGFAULT) FIX ME!!!
    getTangentBasisForMesh(mesh, tangents, bitangents);
    
    //std::cout << "Generating vertex attribute 3 (tangents)..." << std::endl;
    generateAttribute(3, 3, tangents, false);
    
    //std::cout << "Generating vertex attribute 4 (bitangents)..." << std::endl;
    generateAttribute(4, 3, bitangents, false);

    unsigned int indexBufferID;
    glGenBuffers(1, &indexBufferID);
    //std::cout << "Generated Index Buffer: " << indexBufferID << std::endl;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);
    
    //std::cout << "Finished buffer generation!" << std::endl;

    return vaoID;
}