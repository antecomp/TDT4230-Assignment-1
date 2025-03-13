#pragma once
#include "tiny_gltf.h"
#include "mesh.h"
#include "sceneGraph.hpp"

Mesh convertTinyGLTFMesh(const tinygltf::Model &model, const tinygltf::Mesh &gltfMesh);
SceneNode* convertTinyGLTFNode(const tinygltf::Model &model, int nodeIndex);
SceneNode* loadGLBToSceneGraph(const std::string& filename);