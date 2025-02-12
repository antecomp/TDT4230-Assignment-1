#include <chrono>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <SFML/Audio/SoundBuffer.hpp>
#include <utilities/shader.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <utilities/timeutils.h>
#include <utilities/mesh.h>
#include <utilities/shapes.h>
#include <utilities/glutils.h>
#include <SFML/Audio/Sound.hpp>
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

// for debugging
#include "glm/ext.hpp"

#include "utilities/imageLoader.hpp"
#include "utilities/glfont.h"

#include <vector>
#include <cstdlib> // For rand()
#include <ctime>   // For seeding rand()

std::vector<unsigned char> generateNoiseTextureRGBA(int width, int height) {
    std::vector<unsigned char> noiseTexture(width * height * 4);

    std::srand(std::time(nullptr));  // Seed random generator

    for (int i = 0; i < width * height * 4; i += 4) {
        noiseTexture[i + 0] = std::rand() % 256; // R
        noiseTexture[i + 1] = std::rand() % 256; // G
        noiseTexture[i + 2] = std::rand() % 256; // B
        noiseTexture[i + 3] = 255;               // A (fully opaque)
    }

    return noiseTexture;
}



PNGImage fontImage = loadPNGFile("../res/textures/charmap.png");

// Todo: move this elsewhere?
GLuint createTexture(const PNGImage& image) {
    //GLuint textureID;
    unsigned int textureID;

    // Similar syntax and idea to making our VBOs and such...
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Copy raw image data to GPU (I think thats what this does)
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, 
        image.width, image.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data()
    );

    // Automatically generate a MipMap for our texture (wow!)
    // Acting on bound texture, so no extra param needed...
    glGenerateMipmap(GL_TEXTURE_2D);

    // Configure sampling for the texture...
    // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Texels smaller... (This is just pure interpolation - no mipmap)
    // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // MipMap settings : X_MIPMAP_Y   X -> interpolation between mipmaps,  Y -> interpolation for sampling the mipmap itself.
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); // Apply mipmap as texel smaller sampling thingy like this :D

    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Interpolate when texel larger than pixels.



    return textureID;
};

enum KeyFrameAction {
    BOTTOM, TOP
};

#include <timestamps.h>

double padPositionX = 0;
double padPositionZ = 0;

unsigned int currentKeyFrame = 0;
unsigned int previousKeyFrame = 0;

SceneNode* rootNode;
SceneNode* boxNode;
SceneNode* ballNode;
SceneNode* padNode;


SceneNode* textNode;

double ballRadius = 3.0f;

// These are heap allocated, because they should not be initialised at the start of the program
sf::SoundBuffer* buffer;
Gloom::Shader* shader;
sf::Sound* sound;

const glm::vec3 boxDimensions(180, 90, 90);
const glm::vec3 padDimensions(30, 3, 40);

glm::vec3 ballPosition(0, ballRadius + padDimensions.y, boxDimensions.z / 2);
glm::vec3 ballDirection(1, 1, 0.2f);

CommandLineOptions options;

bool hasStarted        = false;
bool hasLost           = false;
bool jumpedToNextFrame = false;
bool isPaused          = false;

bool mouseLeftPressed   = false;
bool mouseLeftReleased  = false;
bool mouseRightPressed  = false;
bool mouseRightReleased = false;

// Modify if you want the music to start further on in the track. Measured in seconds.
const float debug_startTime = 0;
double totalElapsedTime = debug_startTime;
double gameElapsedTime = debug_startTime;

double mouseSensitivity = 1.0;
double lastMouseX = windowWidth / 2;
double lastMouseY = windowHeight / 2;
void mouseCallback(GLFWwindow* window, double x, double y) {
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    glViewport(0, 0, windowWidth, windowHeight);

    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    padPositionX -= mouseSensitivity * deltaX / windowWidth;
    padPositionZ -= mouseSensitivity * deltaY / windowHeight;

    if (padPositionX > 1) padPositionX = 1;
    if (padPositionX < 0) padPositionX = 0;
    if (padPositionZ > 1) padPositionZ = 1;
    if (padPositionZ < 0) padPositionZ = 0;

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
    buffer = new sf::SoundBuffer();
    if (!buffer->loadFromFile("../res/Hall of the Mountain King.ogg")) {
        return;
    }

    options = gameOptions;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetCursorPosCallback(window, mouseCallback);

    shader = new Gloom::Shader();
    shader->makeBasicShader("../res/shaders/simple.vert", "../res/shaders/simple.frag");
    shader->activate();

    // Create meshes
    Mesh pad = cube(padDimensions, glm::vec2(30, 40), true);
    Mesh box = cube(boxDimensions, glm::vec2(90), true, true);
    Mesh sphere = generateSphere(1.0, 40, 40);

    // Fill buffers
    unsigned int ballVAO = generateBuffer(sphere);
    unsigned int boxVAO  = generateBuffer(box);
    unsigned int padVAO  = generateBuffer(pad);

    // Construct scene
    rootNode = createSceneNode();
    boxNode  = createSceneNode();
    padNode  = createSceneNode();
    ballNode = createSceneNode();

    rootNode->children.push_back(boxNode);
    rootNode->children.push_back(padNode);
    rootNode->children.push_back(ballNode);


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
/*
    // Make one of the lights connected to the ball.
    ballNode->children.push_back(SceneLights[0].node);
    SceneLights[0].node->position = glm::vec3(0.0f, 0.0f, 2.0f);

    // Just throw the other lights as children of the scene for now.
    SceneLights[1].node->position = glm::vec3(30.0f, 0.0f, -10.0f);
    boxNode->children.push_back(SceneLights[1].node);
    boxNode->children.push_back(SceneLights[2].node);

    // Light colours
    SceneLights[0].color = glm::vec3(1.0, 0.8, 0.6); // Warm light
    SceneLights[1].color = glm::vec3(0.6, 0.6, 1.0); // Cool light
    SceneLights[2].color = glm::vec3(0.9, 0.9, 0.9); // Neutral white
*/



    // Origin RGB Lights for shadow testing
    SceneLights[0].color = glm::vec3(1.0, 1.0, 1.0);
    SceneLights[0].node->position = glm::vec3(12.0f, 0.0f, 0.0f); 
    SceneLights[1].color = glm::vec3(0.0, 0.0, 1.0); 
    SceneLights[1].node->position = glm::vec3(-12.0f, 0.0f, 0.0f); 
    SceneLights[2].color = glm::vec3(0.0, 1.0, 0.0); 

    boxNode->children.push_back(SceneLights[0].node);
    boxNode->children.push_back(SceneLights[1].node);
    boxNode->children.push_back(SceneLights[2].node);



    boxNode->vertexArrayObjectID  = boxVAO;
    boxNode->VAOIndexCount        = box.indices.size();

    padNode->vertexArrayObjectID  = padVAO;
    padNode->VAOIndexCount        = pad.indices.size();

    ballNode->vertexArrayObjectID = ballVAO;
    ballNode->VAOIndexCount       = sphere.indices.size();

    // I added all this, Mesh stuff for text
    Mesh textMesh = generateTextGeometryBuffer("Hello From OpenGL", 39.0/29, 600);
    unsigned int textVAO = generateBuffer(textMesh);
    textNode = createSceneNode();
    textNode->nodeType = GEOMETRY_2D;
    textNode->vertexArrayObjectID = textVAO;
    textNode->VAOIndexCount = textMesh.indices.size();
    // boxNode->children.push_back(textNode); // Hell
    // textNode->position = glm::vec3(0.0f, 0.0f, 10.0f);
    rootNode->children.push_back(textNode);
    //textNode->position = glm::vec3(0.0f, -5.0f, -90.0f);
    //textNode->position = glm::vec3(0.0f, -0.25f, 0.999999999999999f);
    //textNode->position = glm::vec3(20.0f, 200.0f, 0.0f);
    textNode->position = glm::vec3(windowWidth / 2.0, windowHeight / 2.0, 0.0f);


    // rootNode->children.push_back(textNode); // Hell
    // textNode->position = glm::vec3(0.0f, 0.0f, 0.0f);



    ////////////

    getTimeDeltaSeconds();

    std::cout << fmt::format("Initialized scene with {} SceneNodes.", totalChildren(rootNode)) << std::endl;

    std::cout << "Ready. Click to start!" << std::endl;
}


// Declare these globally so I can reference them easily.
glm::mat4 projection;
glm::vec3 cameraPosition;
glm::mat4 cameraTransform;

void updateFrame(GLFWwindow* window) {


    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double timeDelta = getTimeDeltaSeconds();

    const float ballBottomY = boxNode->position.y - (boxDimensions.y/2) + ballRadius + padDimensions.y;
    const float ballTopY    = boxNode->position.y + (boxDimensions.y/2) - ballRadius;
    const float BallVerticalTravelDistance = ballTopY - ballBottomY;

    const float cameraWallOffset = 30; // Arbitrary addition to prevent ball from going too much into camera

    const float ballMinX = boxNode->position.x - (boxDimensions.x/2) + ballRadius;
    const float ballMaxX = boxNode->position.x + (boxDimensions.x/2) - ballRadius;
    const float ballMinZ = boxNode->position.z - (boxDimensions.z/2) + ballRadius;
    const float ballMaxZ = boxNode->position.z + (boxDimensions.z/2) - ballRadius - cameraWallOffset;

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

    if(!hasStarted) {
        if (mouseLeftPressed) {
            if (options.enableMusic) {
                sound = new sf::Sound();
                sound->setBuffer(*buffer);
                sf::Time startTime = sf::seconds(debug_startTime);
                sound->setPlayingOffset(startTime);
                sound->play();
            }
            totalElapsedTime = debug_startTime;
            gameElapsedTime = debug_startTime;
            hasStarted = true;
        }

        ballPosition.x = ballMinX + (1 - padPositionX) * (ballMaxX - ballMinX);
        ballPosition.y = ballBottomY;
        ballPosition.z = ballMinZ + (1 - padPositionZ) * ((ballMaxZ+cameraWallOffset) - ballMinZ);
    } else {
        totalElapsedTime += timeDelta;
        if(hasLost) {
            if (mouseLeftReleased) {
                hasLost = false;
                hasStarted = false;
                currentKeyFrame = 0;
                previousKeyFrame = 0;
            }
        } else if (isPaused) {
            if (mouseRightReleased) {
                isPaused = false;
                if (options.enableMusic) {
                    sound->play();
                }
            }
        } else {
            gameElapsedTime += timeDelta;
            if (mouseRightReleased) {
                isPaused = true;
                if (options.enableMusic) {
                    sound->pause();
                }
            }
            // Get the timing for the beat of the song
            for (unsigned int i = currentKeyFrame; i < keyFrameTimeStamps.size(); i++) {
                if (gameElapsedTime < keyFrameTimeStamps.at(i)) {
                    continue;
                }
                currentKeyFrame = i;
            }

            jumpedToNextFrame = currentKeyFrame != previousKeyFrame;
            previousKeyFrame = currentKeyFrame;

            double frameStart = keyFrameTimeStamps.at(currentKeyFrame);
            double frameEnd = keyFrameTimeStamps.at(currentKeyFrame + 1); // Assumes last keyframe at infinity

            double elapsedTimeInFrame = gameElapsedTime - frameStart;
            double frameDuration = frameEnd - frameStart;
            double fractionFrameComplete = elapsedTimeInFrame / frameDuration;

            double ballYCoord;

            KeyFrameAction currentOrigin = keyFrameDirections.at(currentKeyFrame);
            KeyFrameAction currentDestination = keyFrameDirections.at(currentKeyFrame + 1);

            // Synchronize ball with music
            if (currentOrigin == BOTTOM && currentDestination == BOTTOM) {
                ballYCoord = ballBottomY;
            } else if (currentOrigin == TOP && currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance;
            } else if (currentDestination == BOTTOM) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * (1 - fractionFrameComplete);
            } else if (currentDestination == TOP) {
                ballYCoord = ballBottomY + BallVerticalTravelDistance * fractionFrameComplete;
            }

            // Make ball move
            const float ballSpeed = 60.0f;
            ballPosition.x += timeDelta * ballSpeed * ballDirection.x;
            ballPosition.y = ballYCoord;
            ballPosition.z += timeDelta * ballSpeed * ballDirection.z;

            // Make ball bounce
            if (ballPosition.x < ballMinX) {
                ballPosition.x = ballMinX;
                ballDirection.x *= -1;
            } else if (ballPosition.x > ballMaxX) {
                ballPosition.x = ballMaxX;
                ballDirection.x *= -1;
            }
            if (ballPosition.z < ballMinZ) {
                ballPosition.z = ballMinZ;
                ballDirection.z *= -1;
            } else if (ballPosition.z > ballMaxZ) {
                ballPosition.z = ballMaxZ;
                ballDirection.z *= -1;
            }

            if(options.enableAutoplay) {
                padPositionX = 1-(ballPosition.x - ballMinX) / (ballMaxX - ballMinX);
                padPositionZ = 1-(ballPosition.z - ballMinZ) / ((ballMaxZ+cameraWallOffset) - ballMinZ);
            }

            // Check if the ball is hitting the pad when the ball is at the bottom.
            // If not, you just lost the game! (hehe)
            if (jumpedToNextFrame && currentOrigin == BOTTOM && currentDestination == TOP) {
                double padLeftX  = boxNode->position.x - (boxDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x);
                double padRightX = padLeftX + padDimensions.x;
                double padFrontZ = boxNode->position.z - (boxDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z);
                double padBackZ  = padFrontZ + padDimensions.z;

                if (   ballPosition.x < padLeftX
                    || ballPosition.x > padRightX
                    || ballPosition.z < padFrontZ
                    || ballPosition.z > padBackZ
                ) {
                    hasLost = true;
                    if (options.enableMusic) {
                        sound->stop();
                        delete sound;
                    }
                }
            }
        }
    }

    // Camera/Transformation Log Starts Here.

    projection = glm::perspective(glm::radians(80.0f), float(windowWidth) / float(windowHeight), 0.1f, 350.f);

    cameraPosition = glm::vec3(0, 2, -20);

    // Some math to make the camera move in a nice way
    float lookRotation = -0.6 / (1 + exp(-5 * (padPositionX-0.5))) + 0.3;
    cameraTransform =
                    glm::rotate(0.3f + 0.2f * float(-padPositionZ*padPositionZ), glm::vec3(1, 0, 0)) *
                    glm::rotate(lookRotation, glm::vec3(0, 1, 0)) *
                    glm::translate(-cameraPosition);

    //glm::mat4 VP = projection * cameraTransform;

    // Move and rotate various SceneNodes
    boxNode->position = { 0, -10, -80 };

    ballNode->position = ballPosition;
    ballNode->scale = glm::vec3(ballRadius);
    ballNode->rotation = { 0, totalElapsedTime*2, 0 };

    padNode->position  = {
        boxNode->position.x - (boxDimensions.x/2) + (padDimensions.x/2) + (1 - padPositionX) * (boxDimensions.x - padDimensions.x),
        boxNode->position.y - (boxDimensions.y/2) + (padDimensions.y/2),
        boxNode->position.z - (boxDimensions.z/2) + (padDimensions.z/2) + (1 - padPositionZ) * (boxDimensions.z - padDimensions.z)
    };

    //updateNodeTransformations(rootNode, VP);
    updateNodeTransformations(rootNode, glm::identity<glm::mat4>());




}

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

// I think a lot of this could be moved to updateNodeTransformations or renderNode, but I think the separation of concerns is a bit more readable.
// Plus, this information is constant between each node, only varies for a frame - so to recalculate it for each node isn't efficient.
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

    // Ball position for shadows.
    GLint ballUniformLoc = shader->getUniformFromName("u_ballPosition");
    // NOTE to self: This looks correct but Im not exactly sure why the transformation matrix should be omitted?
    // Worth coming back to at some point to understand better.
    glUniform3fv(ballUniformLoc, 1, glm::value_ptr(ballNode->position));


    // // asjkhasd
    // GLint performPhongBoolUniformLocation = shader->getUniformFromName("performPhong");
    // // There isn't  "bool" uniform, you use int with 0 or 1
    // glUniform1i(performPhongBoolUniformLocation, false);
}

void renderNode(SceneNode* node) {
    //glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
    glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(projection * cameraTransform * node->currentTransformationMatrix)); // MVP

    // Need M and V and P *ALL* separate bcz we do our phong shading in worldspace.
    glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix)); // M
    glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(cameraTransform)); // V
    glUniformMatrix4fv(6, 1, GL_FALSE, glm::value_ptr(projection)); // P

    glm::mat3 InvTranspose = glm::mat3(glm::transpose(glm::inverse(node->currentTransformationMatrix)));

    //std::cout << glm::to_string(InvTranspose) << std::endl; // Verify matrix is correct

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

    //renderNode(textNode);

}
