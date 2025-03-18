#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D sceneTexture;  // Texture from the first render pass

const float offset = 1.0 / 800.0; // Adjust based on texture size

vec2 offsets[9] = vec2[](
    vec2(-offset,  offset), vec2(0.0,  offset), vec2(offset,  offset),
    vec2(-offset,  0.0),    vec2(0.0,  0.0),    vec2(offset,  0.0),
    vec2(-offset, -offset), vec2(0.0, -offset), vec2(offset, -offset)
);

float sobelKernelX[9] = float[](
    -1,  0,  1,
    -2,  0,  2,
    -1,  0,  1
);

float sobelKernelY[9] = float[](
    1,  2,  1,
    0,  0,  0,
   -1, -2, -1
);

void main()
{
    float edgeX = 0.0;
    float edgeY = 0.0;

    for (int i = 0; i < 9; i++) {
        vec3 color = texture(sceneTexture, TexCoords + offsets[i]).rgb;
        float intensity = dot(color, vec3(0.299, 0.587, 0.114)); // Convert to grayscale
        edgeX += intensity * sobelKernelX[i];
        edgeY += intensity * sobelKernelY[i];
    }

    float edgeStrength = sqrt(edgeX * edgeX + edgeY * edgeY);

    // Invert colors where edges are detected
    vec3 originalColor = texture(sceneTexture, TexCoords).rgb;
    vec3 finalColor = mix(originalColor, vec3(1.0) - originalColor, step(0.2, edgeStrength));

    FragColor = vec4(finalColor, 1.0);
}
