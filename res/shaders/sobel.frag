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


float bayerDither(vec2 coord) {
    coord = mod(floor(coord), 4.0);
    int index = int(coord.x) + int(coord.y) * 4;
    const float bayerMatrix[16] = float[](
        0.0,  8.0,  2.0,  10.0,
        12.0, 4.0,  14.0, 6.0,
        3.0,  11.0, 1.0,  9.0,
        15.0, 7.0,  13.0, 5.0
    );

    return bayerMatrix[index] / 16.0;
}

void main()
{
    vec3 edgeX = vec3(0.0);
    vec3 edgeY = vec3(0.0);

    for (int i = 0; i < 9; i++) {
        float rawDepth = texture(toCameraTexture, TexCoords + offsets[i]).r;
        float adjustedDepth = pow(rawDepth, 0.11); // Adjust curve (closer objects keep stronger depth difference)
        vec3 depth = vec3(adjustedDepth);
        //vec3 depth = vec3(rawDepth)
        vec3 normal = texture(normalTexture, TexCoords + offsets[i]).rgb;
        edgeX += depth * normal * sobelKernelX[i];
        edgeY += depth * normal * sobelKernelY[i];

        //FragColor = vec4(vec3(depth * normal), 1.0);
    }

    float edgeStrength = (length(edgeX) + length(edgeY)) * 0.33; // Compute Sobel magnitude

    // Edge threshold: Strong edges are detected here
    float edgeFactor = smoothstep(0.1, 0.3, edgeStrength); // 0 = no edge, 1 = strong edge
    //float edgeFactor = step(0.12, edgeStrength); // This is the most reliable sensitivity change for edges.

    // Sample the base color texture
    vec3 baseColor = texture(screenTexture, TexCoords).rgb;

    // Invert colors where edges are detected
    vec3 finalColor = mix(baseColor, vec3(1.0) - baseColor, edgeFactor);

    // Dither time
    float ditherThreshhold = bayerDither(vec2(screenWidth * TexCoords.x, screenHeight * TexCoords.y));
    float brightness = dot(finalColor.rgb, vec3(0.299, 0.587, 0.144));
    finalColor = step(ditherThreshhold, vec3(brightness));

    //finalColor = vec3(ditherThreshhold);

    FragColor = vec4(finalColor, 1.0);
}
