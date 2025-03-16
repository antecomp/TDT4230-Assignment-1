#include "glad/glad.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION   // Needed for image loading
#define STB_IMAGE_WRITE_IMPLEMENTATION // Needed for saving textures

#include "gltfUtils.hpp"
#include "glutils.h"
#include <iostream>

// Extremely similar to how createTexture works, just with tinygltfs image format....
unsigned int createGLTexture(const tinygltf::Image &image) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Determine Format From Data
    GLenum format;
    switch(image.component) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default:
            std::cerr << "Unsupported number of components: " << image.component << std::endl;
            return 0;
    }

    // Upload to GPU...
    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        image.width, image.height, 0,
        format, GL_UNSIGNED_BYTE, image.image.data()
    );

    // Mipmaps...
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Unbind the texture
    //glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}




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


        // Texture Coordinates
        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
            const tinygltf::Accessor &uvAccessor = model.accessors[primitive.attributes.at("TEXCOORD_0")];
            const tinygltf::BufferView &uvBufferView = model.bufferViews[uvAccessor.bufferView];
            const tinygltf::Buffer &uvBuffer = model.buffers[uvBufferView.buffer];
        
            const float *uvs = reinterpret_cast<const float *>(
                &uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);
        
            for (size_t i = 0; i < uvAccessor.count; i++) {
                mesh.textureCoordinates.push_back(glm::vec2(uvs[i * 2 + 0], uvs[i * 2 + 1]));
            }
        
            //std::cout << "✅ Loaded " << uvAccessor.count << " texture coordinates." << std::endl;
        } else {
            std::cerr << "⚠ WARNING: No UV maps (TEXCOORD_0) found in GLB file." << std::endl;
        }
    }


    return mesh;
}

SceneNode* convertTinyGLTFNode(const tinygltf::Model &model, int nodeIndex) {
    //std::cout << "Converting GLTF Node: " << nodeIndex << std::endl;

    if (nodeIndex < 0 || nodeIndex >= model.nodes.size()) {
        std::cerr << "Error: Invalid node index " << nodeIndex << std::endl;
        return nullptr;
    }

    const tinygltf::Node &gltfNode = model.nodes[nodeIndex];
    //std::cout << "Fetched node data for node: " << nodeIndex << std::endl;

    //std::cout << "Creating SceneNode..." << std::endl;
    SceneNode* node = createSceneNode();
    if (!node) {
        std::cerr << "Error: Failed to create SceneNode" << std::endl;
        return nullptr;
    }
    //std::cout << "Created SceneNode for node " << nodeIndex << " at " << node << std::endl;

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

    //std::cout << "Transformations applied for node " << nodeIndex << std::endl;

    // Check if the node has a mesh
    if (gltfNode.mesh >= 0) {
        //std::cout << "Node " << nodeIndex << " has a mesh (index " << gltfNode.mesh << "), loading..." << std::endl;
        
        if (gltfNode.mesh >= model.meshes.size()) {
            std::cerr << "Error: Mesh index " << gltfNode.mesh << " out of bounds! Model only has " << model.meshes.size() << " meshes." << std::endl;
            return nullptr;
        }

        Mesh mesh = convertTinyGLTFMesh(model, model.meshes[gltfNode.mesh]);

        //std::cout << "Mesh conversion successful, generating VAO..." << std::endl;
        node->vertexArrayObjectID = generateBuffer(mesh);
        node->VAOIndexCount = mesh.indices.size();
        //std::cout << "VAO generated: " << node->vertexArrayObjectID << ", Index count: " << node->VAOIndexCount << std::endl;

        // tinygltfs own mesh thing holds metadata we need for textures
        const tinygltf::Mesh &gltfMesh = model.meshes[gltfNode.mesh];

        // Texture code goes here.
        int textureID = 0; // Default: No texture applied

        if (!gltfMesh.primitives.empty()) {
            const tinygltf::Primitive &primitive = gltfMesh.primitives[0];

            //std::cout << "🔹 Checking material for texture..." << std::endl;

            if (primitive.material >= 0 && primitive.material < model.materials.size()) {
                const tinygltf::Material &material = model.materials[primitive.material];

                std::cout << "🔹 Found material for primitive." << std::endl;

                if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
                    int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                    //std::cout << "🔹 Material has base color texture index: " << textureIndex << std::endl;

                    if (textureIndex < model.textures.size()) {
                        const tinygltf::Texture &texture = model.textures[textureIndex];
                        //std::cout << "🔹 Found texture in GLTF model at index " << textureIndex << std::endl;

                        if (texture.source >= 0 && texture.source < model.images.size()) {
                            const tinygltf::Image &image = model.images[texture.source];

                            std::cout << "✅ Creating GL texture for image with size: " 
                                      << image.width << "x" << image.height << std::endl;

                            // Upload texture to OpenGL
                            textureID = createGLTexture(image);
                            //std::cout << "✅ Loaded texture ID: " << textureID << " for node " << nodeIndex << std::endl;
                        } else {
                            std::cerr << "❌ ERROR: Texture source index out of bounds! texture.source = " 
                                      << texture.source << ", images.size() = " << model.images.size() << std::endl;
                        }
                    } else {
                        std::cerr << "❌ ERROR: Texture index out of bounds! textureIndex = " 
                                  << textureIndex << ", textures.size() = " << model.textures.size() << std::endl;
                    }
                } else {
                    std::cerr << "⚠ WARNING: No baseColorTexture in material." << std::endl;
                }
            } else {
                std::cerr << "⚠ WARNING: Primitive has no material assigned." << std::endl;
            }
        } else {
            std::cerr << "⚠ WARNING: No primitives found in mesh." << std::endl;
        }

        // Assign texture ID to the scene node
        node->textureID = textureID;
        



    } else {
        std::cout << "Node " << nodeIndex << " has no mesh." << std::endl;
    }


    // Attach child nodes
    //std::cout << "Node " << nodeIndex << " has " << gltfNode.children.size() << " children." << std::endl;
    for (int childIndex : gltfNode.children) {
        std::cout << "Processing child node " << childIndex << std::endl;
        SceneNode* child = convertTinyGLTFNode(model, childIndex);
        if (!child) {
            std::cerr << "Error: Failed to load child node " << childIndex << std::endl;
            continue;
        }
        addChild(node, child);
    }

    //std::cout << "Finished processing node " << nodeIndex << std::endl;



    // Texture test - This prints as expected.
    // for (size_t i = 0; i < model.textures.size(); ++i) {
    //     const tinygltf::Texture &texture = model.textures[i];
    //     std::cout << "Texture " << i << ":" << std::endl;
    
    //     // Get the image index
    //     int imageIndex = texture.source;
    //     if (imageIndex >= 0 && imageIndex < model.images.size()) {
    //         const tinygltf::Image &image = model.images[imageIndex];
    //         std::cout << "  Image URI: " << image.uri << std::endl;
    //         std::cout << "  Image size: " << image.width << "x" << image.height << std::endl;
    //         std::cout << "  Image component: " << image.component << std::endl;
    //         std::cout << "  Image bits: " << image.bits << std::endl;
    //     }
    
    //     // Get the sampler index
    //     int samplerIndex = texture.sampler;
    //     if (samplerIndex >= 0 && samplerIndex < model.samplers.size()) {
    //         const tinygltf::Sampler &sampler = model.samplers[samplerIndex];
    //         std::cout << "  Sampler minFilter: " << sampler.minFilter << std::endl;
    //         std::cout << "  Sampler magFilter: " << sampler.magFilter << std::endl;
    //         std::cout << "  Sampler wrapS: " << sampler.wrapS << std::endl;
    //         std::cout << "  Sampler wrapT: " << sampler.wrapT << std::endl;
    //     }
    // }

    // for (size_t i = 0; i < model.textures.size(); ++i) {
    //     const tinygltf::Texture &texture = model.textures[i];
    //     std::cout << "Texture " << i << ":" << std::endl;
    //     std::cout << "  Source: " << texture.source << std::endl;
    //     std::cout << "  Sampler: " << texture.sampler << std::endl;
    // }
    
    // for (size_t i = 0; i < model.images.size(); ++i) {
    //     const tinygltf::Image &image = model.images[i];
    //     std::cout << "Image " << i << ":" << std::endl;
    //     std::cout << "  URI: " << image.uri << std::endl;
    //     std::cout << "  Size: " << image.width << "x" << image.height << std::endl;
    //     std::cout << "  Components: " << image.component << std::endl;
    // }
    
    // for (size_t i = 0; i < model.materials.size(); ++i) {
    //     const tinygltf::Material &material = model.materials[i];
    //     std::cout << "Material " << i << ":" << std::endl;
    //     if (material.values.find("baseColorTexture") != material.values.end()) {
    //         std::cout << "  Base Color Texture Index: " << material.values.at("baseColorTexture").TextureIndex() << std::endl;
    //     }
    //     if (material.normalTexture.index >= 0) {
    //         std::cout << "  Normal Texture Index: " << material.normalTexture.index << std::endl;
    //     }
    // }







    return node;
}

SceneNode* loadGLBToSceneGraph(const std::string& filename) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    //std::cout << "Loading GLB: " << filename << std::endl;

    bool success = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    if (!success) {
        std::cerr << "Failed to load GLB file: " << err << std::endl;
        return nullptr;
    }

    if (!warn.empty()) std::cerr << "TinyGLTF Warning: " << warn << std::endl;

    //std::cout << "GLB Model loaded successfully!" << std::endl;

    // Check if scene data exists
    if (model.scenes.empty() || model.defaultScene < 0) {
        std::cerr << "Error: No scene found in GLB file" << std::endl;
        return nullptr;
    }

    //std::cout << "Creating root SceneNode..." << std::endl;
    SceneNode* root = createSceneNode();
    if (!root) {
        std::cerr << "Error: Failed to create root SceneNode!" << std::endl;
        return nullptr;
    }

    //std::cout << "Root node created at: " << root << std::endl;

    for (int i : model.scenes[model.defaultScene].nodes) {
        //std::cout << "Processing GLB scene node: " << i << std::endl;
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