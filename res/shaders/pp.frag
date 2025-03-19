#version 450 core
out vec4 FragColor; // Output color

in vec2 TexCoords;  // Input texture coordinates

uniform sampler2D screenTexture;  // Base color texture
uniform sampler2D normalTexture;  // Normal texture

void main() {
    vec3 baseColor = texture(screenTexture, TexCoords).rgb;
    vec3 normalColor = texture(normalTexture, TexCoords).rgb;

    float blendFactor = TexCoords.x; // 0 on the left, 1 on the right

    vec3 blended = mix(baseColor, normalColor, blendFactor);
    
    FragColor = vec4(blended, 1.0);
}
