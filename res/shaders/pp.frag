#version 450 core
out vec4 FragColor; // Output color

in vec2 TexCoords;  // Input texture coordinates from the vertex shader

uniform sampler2D screenTexture; // Texture from the FBO

void main() {
    // Sample the texture and output the color
    FragColor = texture(screenTexture, TexCoords);

    // Example post-processing effect: invert colors
    FragColor = vec4(vec3(1.0 - texture(screenTexture, TexCoords)), 1.0);

    // Example grayscale effect
    // vec3 color = texture(screenTexture, TexCoords).rgb;
    // float gray = dot(color, vec3(0.2126, 0.7152, 0.
}