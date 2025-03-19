#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;  // Original scene colors
uniform sampler2D normalTexture;  // Normal texture for edge detection
uniform sampler2D toCameraTexture; // Holds distance to camera in grayscale.
uniform int screenWidth;
uniform int screenHeight;

float offsetX = 1.0 / screenWidth;
float offsetY = 1.0 / screenHeight;

vec2 offsets[9] = vec2[](
    vec2(-offsetX,  offsetY), vec2(0.0,  offsetY), vec2(offsetX,  offsetY),
    vec2(-offsetX,  0.0),     vec2(0.0,  0.0),     vec2(offsetX,  0.0),
    vec2(-offsetX, -offsetY), vec2(0.0, -offsetY), vec2(offsetX, -offsetY)
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
    vec3 edgeX = vec3(0.0);
    vec3 edgeY = vec3(0.0);

    for (int i = 0; i < 9; i++) {
        float rawDepth = texture(toCameraTexture, TexCoords + offsets[i]).r;
        float adjustedDepth = pow(rawDepth, 0.25); // Adjust curve (closer objects keep stronger depth difference)
        vec3 depth = vec3(adjustedDepth);
        //vec3 depth = vec3(rawDepth)
        vec3 normal = texture(normalTexture, TexCoords + offsets[i]).rgb;
        edgeX += depth * normal * sobelKernelX[i];
        edgeY += depth * normal * sobelKernelY[i];
    }

    float edgeStrength = (length(edgeX) + length(edgeY)) * 0.33; // Compute Sobel magnitude

    // Edge threshold: Strong edges are detected here
    float edgeFactor = smoothstep(0.1, 0.3, edgeStrength); // 0 = no edge, 1 = strong edge

    // Sample the base color texture
    vec3 baseColor = texture(screenTexture, TexCoords).rgb;

    // Invert colors where edges are detected
    vec3 finalColor = mix(baseColor, vec3(1.0) - baseColor, edgeFactor);

    FragColor = vec4(finalColor, 1.0);
}
