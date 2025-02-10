#include <iostream>
#include "glfont.h"

Mesh generateTextGeometryBuffer(std::string text, float characterHeightOverWidth, float totalTextWidth) {
    float characterWidth = totalTextWidth / float(text.length());
    float characterHeight = characterHeightOverWidth * characterWidth;

    unsigned int vertexCount = 4 * text.length();
    unsigned int indexCount = 6 * text.length();

    Mesh mesh;

    mesh.vertices.resize(vertexCount);
    mesh.indices.resize(indexCount);

    for(unsigned int i = 0; i < text.length(); i++)
    {
        float baseXCoordinate = float(i) * characterWidth;
        // Texture coords... (I added this)
        char character = text[i];
        //int charIndex = character - 32; // nevermind the texture has 31 X's to do this for us lol
        int charCol = character;
        float u_min = charCol * characterWidth;
        float u_max = (charCol + 1) * characterWidth;
        // Might need to flip these if I messed up the coordinate system in my head :)
        float v_min = 1; // always 1 I think? The top
        float v_max = 0; // always 0 I think? The bottom.

        mesh.textureCoordinates.at(4 * i + 0) = {u_min, v_min}; // BL
        mesh.textureCoordinates.at(4 * i + 1) = {u_max, v_min}; // BR
        mesh.textureCoordinates.at(4 * i + 2) = {u_max, v_max}; // TR
        mesh.textureCoordinates.at(4 * i + 3) = {u_min, v_max}; // TL


        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 1) = {baseXCoordinate + characterWidth, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};

        mesh.vertices.at(4 * i + 0) = {baseXCoordinate, 0, 0};
        mesh.vertices.at(4 * i + 2) = {baseXCoordinate + characterWidth, characterHeight, 0};
        mesh.vertices.at(4 * i + 3) = {baseXCoordinate, characterHeight, 0};


        mesh.indices.at(6 * i + 0) = 4 * i + 0;
        mesh.indices.at(6 * i + 1) = 4 * i + 1;
        mesh.indices.at(6 * i + 2) = 4 * i + 2;
        mesh.indices.at(6 * i + 3) = 4 * i + 0;
        mesh.indices.at(6 * i + 4) = 4 * i + 2;
        mesh.indices.at(6 * i + 5) = 4 * i + 3;

    }

    return mesh;
}