#include <GLFW/glfw3.h>
#include <cstdlib>
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
#include "glm/trigonometric.hpp"
#include "sceneGraph.hpp"
#include "utilities/window.hpp"
#define GLM_ENABLE_EXPERIMENTAL // Required by transform
#include <glm/gtx/transform.hpp>

#include "utilities/textureUtils.h"
#include "utilities/gltfUtils.hpp"

enum KeyFrameAction {
    BOTTOM, TOP
};

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* textNode;

// Global transforms for ez (lazy) reference.
glm::mat4 projection;
glm::vec3 cameraPosition;


// Camera Stuff
SceneNode* bodyNode; // Holds "yaw"
SceneNode* headNode; // Holds "pitch"
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
glm::mat4 cameraTransform;
float FOV = glm::radians(45.0);
float horizontalFOV = 2.0f * glm::atan(glm::tan(FOV / 2.0f) * ((float)windowWidth / windowHeight));

#define NUM_LIGHT_SOURCES 3
// Node data for easy use with the existing scene graph layout
struct SceneLight {
    int id;
    SceneNode* node;
    glm::vec3 color;
};

SceneLight SceneLights[NUM_LIGHT_SOURCES];

// These are heap allocated, because they should not be initialised at the start of the program
Gloom::Shader* shader;

CommandLineOptions options;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

double mouseSensitivity = 1.0;
void mouseCallback(GLFWwindow* window, double x, double y) {
    static bool firstMouse = true;
    static double lastX = windowWidth / 2;
    static double lastY = windowHeight / 2;

    if (firstMouse) {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    float deltaX = float(x - lastX) * mouseSensitivity;
    float deltaY = float(y - lastY) * mouseSensitivity;

    lastX = x;
    lastY = y;

    // Adjust yaw (body) and pitch (head)
    cameraYaw -= deltaX * 0.002f; // Inverted X (right is positive)
    cameraPitch -= deltaY * 0.002f; // Inverted Y (up is negative)

    // Clamp pitch to avoid flipping
    cameraPitch = glm::clamp(cameraPitch, -glm::radians(89.0f), glm::radians(89.0f));
}

void updateCamera() {
    bodyNode->rotation.y = cameraYaw;
    headNode->rotation.x = cameraPitch;

    cameraPosition = bodyNode->position;
    // View matrix = inverse of cameras world transform.
    cameraTransform = glm::inverse(headNode->currentTransformationMatrix);
}


// Keyboard Movement
bool keyW = false, keyA = false, keyS = false, keyD = false;
bool keySpace = false, keyCtrl = false;
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_RELEASE) {
        bool isPressed = (action == GLFW_PRESS);

        switch (key) {
            case GLFW_KEY_W: keyW = isPressed; break;
            case GLFW_KEY_A: keyA = isPressed; break;
            case GLFW_KEY_S: keyS = isPressed; break;
            case GLFW_KEY_D: keyD = isPressed; break;
            case GLFW_KEY_SPACE: keySpace = isPressed; break;
            case GLFW_KEY_LEFT_CONTROL: keyCtrl = isPressed; break;
        }
    }
}

void updateMovement(float deltaTime) {
    float moveSpeed = 20.0f * deltaTime;

    glm::vec3 moveDirection = glm::vec3(0.0f);

    glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), cameraYaw, glm::vec3(0, 1, 0));
    glm::vec3 forward = glm::vec3(yawRotation * glm::vec4(0, 0, -1, 0)); // -Z spun by yaw
    glm::vec3 right   = glm::vec3(yawRotation * glm::vec4(1, 0, 0, 0));  // +X spun by yaw.


    if (keyW) moveDirection += forward;
    if (keyS) moveDirection -= forward;
    if (keyA) moveDirection -= right;
    if (keyD) moveDirection += right;

    // Up/Down is just relative to world space instead.
    if (keySpace) moveDirection += glm::vec3(0, 1, 0);
    if (keyCtrl) moveDirection -= glm::vec3(0, 1, 0);

    // Apply movement
    if (glm::length(moveDirection) > 0) {
        bodyNode->position += glm::normalize(moveDirection) * moveSpeed;
    }
}



void initGame(GLFWwindow* window, CommandLineOptions gameOptions) {
    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetKeyCallback(window, keyCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");

    rootNode = createSceneNode();


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

    rootNode->children.push_back(SceneLights[0].node);
    rootNode->children.push_back(SceneLights[1].node);
    rootNode->children.push_back(SceneLights[2].node);

    // Camera stuff
    bodyNode = createSceneNode();
    headNode = createSceneNode();

    bodyNode->nodeType = FIRST_PERSON_CAMERA;
    headNode->nodeType = FIRST_PERSON_CAMERA;
    bodyNode->children.push_back(headNode);

    bodyNode->position = glm::vec3(0, 10, 25); // SYNONYMOUS WITH INITIAL CAMERA POSITION!!! 
    headNode->position = glm::vec3(0, 0, 0);
    rootNode->children.push_back(bodyNode);



    // Add Test Models.
    SceneNode* well = loadGLBToSceneGraph("../res/gtlf/well_baked.glb");
    rootNode->children.push_back(well);
    well->position = glm::vec3(-10.0, -10.0, -10.0);
    well->scale = glm::vec3(5.0, 5.0, 5.0);

    SceneNode* teapot = loadGLBToSceneGraph("../res/gtlf/teapot_smooth.glb");
    rootNode->children.push_back(teapot);
    teapot->position = glm::vec3(10.0, -10.0, -10.0);
    teapot->scale = glm::vec3(2.0, 2.0, 2.0);
    teapot->rotation = glm::vec3(0.0, 3.1, 0.0);

    SceneNode* chair = loadGLBToSceneGraph("../res/gtlf/chair.glb");
    rootNode->children.push_back(chair);
    chair->position = glm::vec3(-40.0, -10.0, -10.0);
    chair->scale = glm::vec3(3.0, 3.0, 3.0);

    Mesh box = cube(glm::vec3(20, 20, 20), glm::vec2(50), true, false);
    unsigned int boxVAO  = generateBuffer(box);
    SceneNode* boxNode  = createSceneNode();
    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();
    boxNode->position = glm::vec3(-70.0, 10.0, -10.0);
    rootNode->children.push_back(boxNode);


    Mesh box2 = cube(glm::vec3(10, 10, 10), glm::vec2(50), true, false);
    unsigned int box2VAO  = generateBuffer(box2);
    SceneNode* box2Node  = createSceneNode();
    box2Node->vertexArrayObjectID  = box2VAO;
    box2Node->VAOIndexCount        = box2.indices.size();
    box2Node->position = glm::vec3(-70.0, 10.0, -40.0);
    rootNode->children.push_back(box2Node);


    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;
}

void updateFrame(GLFWwindow* window) {

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

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

    // Camera/Transformation
    projection = glm::perspective(FOV, float(windowWidth) / float(windowHeight), 0.1f, 350.f);
    updateMovement(timeDelta);
    updateCamera();

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

    for(SceneNode* child : node->children) {
        updateNodeTransformations(child, node->currentTransformationMatrix);
    }
}

void uploadUniforms() {

    // Uniform Struct format, what gets sent to the frag shader. SceneLight transformed to this by uploadUniforms.
    struct LightSource {
        glm::vec3 position;
        glm::vec3 color;
    };

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

    // Test - Upload an ID to differentiate every
    glUniform1i(shader->getUniformFromName("objectID"), node->objectID);


    switch(node->nodeType) {
        case GEOMETRY:
            if(node->vertexArrayObjectID != -1) {
                glBindVertexArray(node->vertexArrayObjectID);

                glBindTexture(GL_TEXTURE_2D, node->textureID);
                glBindTextureUnit(1, node->textureID);

                glDrawElements(GL_TRIANGLES, node->VAOIndexCount, GL_UNSIGNED_INT, nullptr);

                // Reset to avoid unintended bindings
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindTextureUnit(1, 0);
            }
            break;
        case POINT_LIGHT: break;
        case SPOT_LIGHT: break;
        case GEOMETRY_2D: 
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

                glBindTexture(GL_TEXTURE_2D, 0);
                glBindTextureUnit(1, 0);
                glBindTextureUnit(2, 0);


            }
        break;
        case FIRST_PERSON_CAMERA:
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

    shader->activate();

    uploadUniforms();

    renderNode(rootNode);

    shader->deactivate();
}