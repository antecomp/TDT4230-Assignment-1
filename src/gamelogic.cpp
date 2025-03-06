#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fmt/format.h>
#include "gamelogic.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/matrix.hpp"
#include "sceneGraph.hpp"
#include "utilities/window.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

#include "utilities/textureUtils.h"

PNGImage fontImage = loadPNGFile("../res/textures/charmap.png");

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;

SceneNode* textNode;

// Declare these globally so I can reference them easily.
glm::mat4 projection;
glm::vec3 cameraPosition;
glm::mat4 cameraTransform;


// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* shader;

const glm::vec3 boxDimensions(180, 90, 90);

CommandLineOptions options;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    // double deltaX = x - lastMouseX;
    // double deltaY = y - lastMouseY;

    glfwSetCursorPos(window, windowWidth / 2, windowHeight / 2);
}

#define NUM_LIGHT_SOURCES 3
// Node data for easy use with the existing scene graph layout
struct SceneLight {
    int id;
    SceneNode* node;
    glm::vec3 color;
};

// Uniform Struct format, what gets sent to the frag shader. SceneLight transformed to this by uploadUniforms.
struct LightSource {
    glm::vec3 position;
    glm::vec3 color;
};

SceneLight SceneLights[NUM_LIGHT_SOURCES];

void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Create meshes
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);

    // Fill buffers
    unsigned int boxVAO  = generateBuffer(box);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();

    rootNode->children.push_back(boxNode);


    // Change box to be normal map type (I added this)
    boxNode->nodeType = NORMAL_MAPPED_GEOMETRY;

    // Load and apply texture/normal map for box, store them there...
    PNGImage bricksColImage = loadPNGFile("../res/textures/Brick03_col.png");
    auto brickColTextureID = createTexture(bricksColImage);
    boxNode->textureID = brickColTextureID;

    PNGImage bricksNormImage = loadPNGFile("../res/textures/Brick03_nrm.png");
    auto brickNormTextureID = createTexture(bricksNormImage);
    boxNode->normalMapTextureID = brickNormTextureID;



// Create lights and add them to scene
    for(int i = 0; i < NUM_LIGHT_SOURCES; i++) {
        SceneLights[i].id = i;
        SceneNode* node = createSceneNode();
        node->nodeType = POINT_LIGHT;
        SceneLights[i].node = node;
    }

    // Basic white lights for testing normal map.
    SceneLights[0].color = glm::vec3(1.0, 1.0, 1.0);
    SceneLights[0].node->position = glm::vec3(12.0f, 0.0f, 0.0f); 
    SceneLights[1].color = glm::vec3(1.0, 1.0, 1.0); 
    SceneLights[1].node->position = glm::vec3(-24.0f, 0.0f, 0.0f); 
    SceneLights[2].color = glm::vec3(1.0, 1.0, 1.0); 

    boxNode->children.push_back(SceneLights[0].node);
    boxNode->children.push_back(SceneLights[1].node);
    boxNode->children.push_back(SceneLights[2].node);



    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    // I added all this, Mesh stuff for text
    Mesh textMesh = generateTextGeometryBuffer("TDT4230 Final Project", 39.0/29, 500);
    unsigned int textVAO = generateBuffer(textMesh);
    textNode = createSceneNode();
    textNode->nodeType = GEOMETRY_2D;
    textNode->vertexArrayObjectID = textVAO;
    textNode->VAOIndexCount = textMesh.indices.size();
    rootNode->children.push_back(textNode);
    textNode->position = glm::vec3(40, windowHeight - 40.0, 0.0f);

    ////////////

    //getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;
}

void updateFrame(GLFWwindow* window) {

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    //double timeDelta = getTimeDeltaSeconds();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
        mouseLeftPressed = true;
        mouseLeftReleased = false;
    } else {
        mouseLeftReleased = mouseLeftPressed;
        mouseLeftPressed = false;
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
        mouseRightPressed = true;
        mouseRightReleased = false;
    } else {
        mouseRightReleased = mouseRightPressed;
        mouseRightPressed = false;
    }

    // Camera/Transformation Log Starts Here.


    float time = glfwGetTime(); // Alternatively....

    float rotationAmount = sin(time) * 0.1f; // Small oscillation (verifying we have a proper frame update)

    projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    //cameraPosition = glm::vec3(0, 2, -20);
    cameraPosition = glm::vec3(0, 10, 10);

    cameraTransform = 
        glm::rotate(rotationAmount, glm::vec3(0, 1, 0)) * // Rotate around Y-axis
        glm::lookAt(
            cameraPosition, 
            glm::vec3(0,5,0), 
            glm::vec3(0,1,0)
        ) 
        * glm::translate(-cameraPosition);

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    // Traverse scene graph and reduce transformations together.
    updateNodeTransformations(rootNode, glm::identity<glm::mat4>());

}

// TODO: Move to separate file?
void updateNodeTransformations(SceneNode* node, glm::mat4 transformationThusFar) {
    glm::mat4 transformationMatrix =
              glm::translate(node->position)
            * glm::translate(node->referencePoint)
            * glm::rotate(node->rotation.y, glm::vec3(0,1,0))
            * glm::rotate(node->rotation.x, glm::vec3(1,0,0))
            * glm::rotate(node->rotation.z, glm::vec3(0,0,1))
            * glm::scale(node->scale)
            * glm::translate(-node->referencePoint);

    node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

    switch(node->nodeType) {
        case GEOMETRY: break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        case GEOMETRY_2D: break;
        case NORMAL_MAPPED_GEOMETRY: break;
    }        

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

// TODO: Seperate File?
void uploadUniforms() {
    LightSource lightData[NUM_LIGHT_SOURCES];

    for(int i = 0; i < NUM_LIGHT_SOURCES; ++i) {
        // Collapse/Apply transform onto position for forwaring first...
        glm::vec4 transformedPosition = SceneLights[i].node->currentTransformationMatrix * glm::vec4(SceneLights[i].node->position, 1.0);
        lightData[i].position = glm::vec3(transformedPosition);
        lightData[i].color = SceneLights[i].color;
    }

    // Upload each struct member separately
    for (int i = 0; i < NUM_LIGHT_SOURCES; ++i) {
        std::string posUniform = fmt::format("lightSources[{}].position", i);
        std::string colorUniform = fmt::format("lightSources[{}].color", i);

        GLint posLocation = shader->getUniformFromName(posUniform);
        GLint colorLocation = shader->getUniformFromName(colorUniform);

        glUniform3fv(posLocation, 1, glm::value_ptr(lightData[i].position));
        glUniform3fv(colorLocation, 1, glm::value_ptr(lightData[i].color));
    }

    // Camera position for reflections
    //GLint cameraUniformLocation = glGetUniformLocation(shader->get(), "u_cameraPosition"); // Old way I found online before learning about shaders helper func.
    GLint cameraUniformLocation = shader->getUniformFromName("u_cameraPosition");
    glUniform3fv(cameraUniformLocation, 1, glm::value_ptr(cameraPosition));
}

void renderNode(SceneNode* node) {
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projection * cameraTransform * node->currentTransformationMatrix)); // MVP

    // Need M and V and P *ALL* separate bcz we do our phong shading in worldspace.
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix)); // M
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(cameraTransform)); // V
    glUniformMatrix4fv(6, 1, GL_FALSE, glm::value_ptr(projection)); // P

    glm::mat3 InvTranspose = glm::mat3(glm::transpose(glm::inverse(node->currentTransformationMatrix)));

    // Inverse of the transpose + only top 3x3 matrix (we dont translate our normals.)
    glUniformMatrix3fv(7, 1, GL_FALSE, glm::value_ptr(InvTranspose));


    // Toggle between enabling phong or not, default is yes (turned off in switch statement.)
    GLint is2DULoc = shader->getUniformFromName("is2D");
    glUniform1i(is2DULoc, false); // We use 1int for bools, tldr it sends it to the gpu as 0/1

    // Similarly, toggle between modes for our normal mapping stuff...
    GLuint hasNormalMappedGeomLoc = shader->getUniformFromName("hasNormalMappedGeom");
    glUniform1i(hasNormalMappedGeomLoc, false);

    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
            break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        case GEOMETRY_2D: 
            if(node->vertexArrayObjectID != -1) { // Note to self: I should probably extract a lot of this behavior elsewhere!
                
                // Positioning/Shading Stuf...
                glm::mat4 orthoProjection = glm::ortho(
                    0.0f, (float)windowWidth,  // Left to Right
                    0.0f, (float)windowHeight,   // Bottom to Top (flipped because OpenGL NDC has -Y up)
                    -1.0f, 1.0f                 // Near and Far (we only need a small depth range)
                );
                GLuint orthoULoc = shader->getUniformFromName("Ortho");
                glUniformMatrix4fv(orthoULoc, 1, GL_FALSE, glm::value_ptr(orthoProjection));
                glUniform1i(is2DULoc, true);

                // Texturing stuff
                auto textTextureID = createTexture(fontImage);
                glBindTexture(GL_TEXTURE_2D, textTextureID); // idk why I had this, works without. Keeping comment just in case.
                glBindTextureUnit(0, textTextureID);


                // (DEBUG/TEST) THIS SHOWS NOISE AS EXPECTED
                // std::vector<unsigned char> noiseData = generateNoiseTextureRGBA(128, 128);
                // GLuint noiseTextureID;
                // glGenTextures(1, &noiseTextureID);
                // glBindTexture(GL_TEXTURE_2D, noiseTextureID);
                // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, noiseData.data());
                // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                // glBindTexture(GL_TEXTURE_2D, noiseTextureID);
                // glBindTextureUnit(0, noiseTextureID);



                glBindVertexArray(node->vertexArrayObjectID); // totally didnt forget to put this and struggle to debug for hours :^)
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
        break;
        case NORMAL_MAPPED_GEOMETRY:
            if(node->vertexArrayObjectID != -1) {

                glUniform1i(hasNormalMappedGeomLoc, true);

                glBindTexture(GL_TEXTURE_2D, node->textureID);
                glBindTextureUnit(1, node->textureID);

                glBindTexture(GL_TEXTURE_2D, node->normalMapTextureID);
                glBindTextureUnit(2, node->normalMapTextureID);

                glBindVertexArray(node->vertexArrayObjectID);
                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);
            }
        break;
    }

    for(SceneNode* child : node->children) {
        renderNode(child);
    }
}

void renderFrame(GLFWwindow* window) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    uploadUniforms();

    renderNode(rootNode);
}
