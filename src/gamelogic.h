#pragma once

#include <utilities/window.hpp>
#include "sceneGraph.hpp"

//void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar);
void updateNodeTransformations(SceneNode* node, glm::mat4 M, glm::mat4 V);
void initGame(GLFWwindow* window, CommandLineOptions options);
void updateFrame(GLFWwindow* window);
void renderFrame(GLFWwindow* window);