#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION   // Needed for image loading
#define STB_IMAGE_WRITE_IMPLEMENTATION // Needed for saving textures

#include "gltfUtils.hpp"
#include "glutils.h"
#include <iostream>

Mesh convertTinyGLTFMesh(const tinygltf::Model &model, const tinygltf::Mesh &gltfMesh) {
    Mesh mesh;

    for (const auto &primitive : gltfMesh.primitives) {
        const tinygltf::Accessor &posAccessor = model.accessors[primitive.attributes.at("POSITION")];
        const tinygltf::BufferView &posBufferView = model.bufferViews[posAccessor.bufferView];
        const tinygltf::Buffer &posBuffer = model.buffers[posBufferView.buffer];

        // Extract vertices
        const float *positions = reinterpret_cast<const float *>(
            &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset]);
        for (size_t i = 0; i < posAccessor.count; i++) {
            mesh.vertices.push_back(glm::vec3(
                positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]
            ));
        }

        // Extract normals if available
        if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
            const tinygltf::Accessor &normAccessor = model.accessors[primitive.attributes.at("NORMAL")];
            const tinygltf::BufferView &normBufferView = model.bufferViews[normAccessor.bufferView];
            const tinygltf::Buffer &normBuffer = model.buffers[normBufferView.buffer];

            const float *normals = reinterpret_cast<const float *>(
                &normBuffer.data[normBufferView.byteOffset + normAccessor.byteOffset]);
            for (size_t i = 0; i < normAccessor.count; i++) {
                mesh.normals.push_back(glm::vec3(
                    normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]
                ));
            }
        }

        // Extract indices
        if (primitive.indices >= 0) {
            const tinygltf::Accessor &indexAccessor = model.accessors[primitive.indices];
            const tinygltf::BufferView &indexBufferView = model.bufferViews[indexAccessor.bufferView];
            const tinygltf::Buffer &indexBuffer = model.buffers[indexBufferView.buffer];

            const unsigned short *indices = reinterpret_cast<const unsigned short *>(
                &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
            for (size_t i = 0; i < indexAccessor.count; i++) {
                mesh.indices.push_back(indices[i]);
            }
        }
    }


    return mesh;
}

SceneNode* convertTinyGLTFNode(const tinygltf::Model &model, int nodeIndex) {
    std::cout << "Converting GLTF Node: " << nodeIndex << std::endl;

    if (nodeIndex < 0 || nodeIndex >= model.nodes.size()) {
        std::cerr << "Error: Invalid node index " << nodeIndex << std::endl;
        return nullptr;
    }

    const tinygltf::Node &gltfNode = model.nodes[nodeIndex];
    std::cout << "Fetched node data for node: " << nodeIndex << std::endl;

    std::cout << "Creating SceneNode..." << std::endl;
    SceneNode* node = createSceneNode();
    if (!node) {
        std::cerr << "Error: Failed to create SceneNode" << std::endl;
        return nullptr;
    }
    std::cout << "Created SceneNode for node " << nodeIndex << " at " << node << std::endl;

    // Check for transformations
    if (!gltfNode.translation.empty()) {
        node->position = glm::vec3(
            gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]
        );
    }
    if (!gltfNode.rotation.empty()) {
        glm::quat rotation(
            gltfNode.rotation[3], gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2]
        );
        node->rotation = glm::eulerAngles(rotation);
    }
    if (!gltfNode.scale.empty()) {
        node->scale = glm::vec3(
            gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]
        );
    }

    std::cout << "Transformations applied for node " << nodeIndex << std::endl;

    // Check if the node has a mesh
    if (gltfNode.mesh >= 0) {
        std::cout << "Node " << nodeIndex << " has a mesh (index " << gltfNode.mesh << "), loading..." << std::endl;
        
        if (gltfNode.mesh >= model.meshes.size()) {
            std::cerr << "Error: Mesh index " << gltfNode.mesh << " out of bounds! Model only has " << model.meshes.size() << " meshes." << std::endl;
            return nullptr;
        }

        Mesh mesh = convertTinyGLTFMesh(model, model.meshes[gltfNode.mesh]);

        std::cout << "Mesh conversion successful, generating VAO..." << std::endl;
        node->vertexArrayObjectID = generateBuffer(mesh);
        node->VAOIndexCount = mesh.indices.size();
        std::cout << "VAO generated: " << node->vertexArrayObjectID << ", Index count: " << node->VAOIndexCount << std::endl;
    } else {
        std::cout << "Node " << nodeIndex << " has no mesh." << std::endl;
    }

    // Attach child nodes
    std::cout << "Node " << nodeIndex << " has " << gltfNode.children.size() << " children." << std::endl;
    for (int childIndex : gltfNode.children) {
        std::cout << "Processing child node " << childIndex << std::endl;
        SceneNode* child = convertTinyGLTFNode(model, childIndex);
        if (!child) {
            std::cerr << "Error: Failed to load child node " << childIndex << std::endl;
            continue;
        }
        addChild(node, child);
    }

    std::cout << "Finished processing node " << nodeIndex << std::endl;

    return node;
}

SceneNode* loadGLBToSceneGraph(const std::string& filename) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    std::cout << "Loading GLB: " << filename << std::endl;

    bool success = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    if (!success) {
        std::cerr << "Failed to load GLB file: " << err << std::endl;
        return nullptr;
    }

    if (!warn.empty()) std::cerr << "TinyGLTF Warning: " << warn << std::endl;

    std::cout << "GLB Model loaded successfully!" << std::endl;

    // Check if scene data exists
    if (model.scenes.empty() || model.defaultScene < 0) {
        std::cerr << "Error: No scene found in GLB file" << std::endl;
        return nullptr;
    }

    std::cout << "Creating root SceneNode..." << std::endl;
    SceneNode* root = createSceneNode();
    if (!root) {
        std::cerr << "Error: Failed to create root SceneNode!" << std::endl;
        return nullptr;
    }

    std::cout << "Root node created at: " << root << std::endl;

    for (int i : model.scenes[model.defaultScene].nodes) {
        std::cout << "Processing GLB scene node: " << i << std::endl;
        SceneNode* child = convertTinyGLTFNode(model, i);
        if (!child) {
            std::cerr << "Error: Failed to convert node " << i << std::endl;
            continue;
        }
        addChild(root, child);
    }

    std::cout << "Finished loading GLB model!" << std::endl;

    return root;
}