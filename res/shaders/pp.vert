#version 450 core
layout (location = 0) in vec2 aPos;       // Vertex position
layout (location = 1) in vec2 aTexCoords; // Texture coordinates

out vec2 TexCoords; // Output texture coordinates to the fragment shader

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0); // Set vertex position (full-screen quad)
    TexCoords = aTexCoords;             // Pass texture coordinates to fragment shader
}