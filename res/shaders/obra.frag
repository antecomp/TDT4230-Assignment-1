#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D screenTexture;  // Original scene colors
uniform sampler2D normalTexture;  // Normal texture for edge detection. Note this is a [0,1] encoding.
uniform sampler2D toCameraTexture; // Holds distance to camera in grayscale.
uniform usampler2D objectIDMap; // Fragments associated object ID.
uniform int screenWidth;
uniform int screenHeight;

uniform float ditherOffsetX;
uniform float ditherOffsetY;

float offsetX = 1.0 / screenWidth;
float offsetY = 1.0 / screenHeight;

vec3 debugColorFromObjectID(uint id) {
    return vec3(
        float((id * 47) % 256) / 255.0,
        float((id * 97) % 256) / 255.0,
        float((id * 151) % 256) / 255.0
    );
}

const float normal_diff_threshold = 0.5;

// [0,1] to [-1,1]. Obv if I change how the normal is sent we can change this back.
vec3 unpackNormal(vec3 encoded) {
    return encoded * 2.0 - 1.0;
}

bool isEdge(vec3 normalOfCenter, vec3 normalOfOther, uint idOfCenter, uint idOfOther) {
    float normalDiff = length(normalOfCenter - normalOfOther);

    if(normalDiff > normal_diff_threshold)
        return true;
    else if (idOfCenter != idOfOther)
        return true;
    else
        return false;
}

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
   ivec2 uv = ivec2(gl_FragCoord.xy);

   vec3 centerNormal = unpackNormal(texture(normalTexture, TexCoords).rgb);
   uint centerID = texelFetch(objectIDMap, uv, 0).r;

    // Neighbours... 
    ivec2 upUV    = uv + ivec2(0, 1);
    ivec2 leftUV  = uv - ivec2(1, 0);
    vec3 upNormal    = unpackNormal(texelFetch(normalTexture, upUV, 0).rgb);
    uint upID        = texelFetch(objectIDMap, upUV, 0).r;
    vec3 leftNormal  = unpackNormal(texelFetch(normalTexture, leftUV, 0).rgb);
    uint leftID      = texelFetch(objectIDMap, leftUV, 0).r;

    bool edge = false
        || isEdge(centerNormal, upNormal, centerID, upID)
        || isEdge(centerNormal, leftNormal, centerID, leftID);

    // Texture map representing the render of the scene...
    vec3 baseColor = texture(screenTexture, TexCoords).rgb;

    // Invert Colour At Edges
    if(edge) {
        baseColor = 1.0 - baseColor;
    }

    // Dither time
    float ditherThreshhold = bayerDither(vec2(screenWidth * TexCoords.x + ditherOffsetX, screenHeight * TexCoords.y + ditherOffsetY));
    float brightness = dot(baseColor.rgb, vec3(0.299, 0.587, 0.144));
    baseColor = step(ditherThreshhold, vec3(brightness));

    FragColor = vec4(baseColor, 1.0);
}
