#version 430 core

#define NUM_LIGHT_SOURCES 3

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }


vec3 ambientIntensity = vec3(0.2, 0.1, 0.1);
vec3 diffuseColour = vec3(0.4, 0.4, 0.4); // SHould be a uniform later :)
vec3 specularColour = vec3(1.0, 1.0, 1.0); // Specular reflection color (also make uniform later??)

uniform vec3 lightPositions[NUM_LIGHT_SOURCES];
uniform vec3 u_cameraPosition;

in vec3 fragWSPosition; 

void main()
{
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    for(int i = 0; i < NUM_LIGHT_SOURCES; i++) {
        vec3 lightDir = normalize(lightPositions[i] - fragWSPosition);

        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0); 
        totalDiffuse += diff * diffuseColour;

        // Specular Reflect
        vec3 reflectDir = reflect(-lightDir, normal); // For ease we're directly reflecting back towards light
        vec3 viewDir = normalize(u_cameraPosition - fragWSPosition); // View direction toward the camera

        // Specular intensity = reflected vector dot view (surface 2 eye)
        // If the specular factor is negative, you should set it to 0.
        // 32.0 is the shininess factor (hard coded, will make uniform l8r).
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
        totalSpecular += spec * specularColour;
    }

    vec3 finalColour = ambientIntensity + totalDiffuse + totalSpecular;

    color = vec4(finalColour, 1.0);
}


/*
    For each light we need:
    [normalized] light direction vector (vector from WS fragment position to WS light position)
    diffuse intensity = dot product of light direction vector and surface normal (WS)
    diffuse colour = diffuse intensity * some surface diffuse colour.

    Add all comps together, and normalize(???)
*/




// Old basic test
// void main()
// {
//     // Normalize again because interpolation can distort normals.
//     color = vec4(0.5 * normalize(normal) + 0.5, 1.0);
//     //color = vec4(0.5, 0.0, 0.0, 1.0);
// }




// CHATGPT GENERATED DEBUG SHADER FOR VERIFYING LIGHT POSITION.
// #define NUM_LIGHT_SOURCES 3
// out vec4 FragColor;
// uniform vec3 lightPositions[NUM_LIGHT_SOURCES];

// //uniform layout(location = 4) mat4 M;
// uniform layout(location = 5) mat4 view;
// uniform layout(location = 6) mat4 projection;

// in layout(location = 0) vec3 Normal;

// void main()
// {
//     // Base shading using the normal
//     vec4 baseColor = vec4(0.5 * Normal + 0.5, 1.0); // Simple shading

//     // Debug light visualization
//     vec3 debugColor = vec3(0.0);

//     for (int i = 0; i < NUM_LIGHT_SOURCES; ++i) {
//         // Transform the light position into clip space
//         vec4 lightClipPos = projection * view * vec4(lightPositions[i], 1.0);
//         vec3 lightScreenPos = lightClipPos.xyz / lightClipPos.w; // Perspective divide

//         // Map clip space [-1, 1] to screen space [0, screenWidth/screenHeight]
//         lightScreenPos.xy = (lightScreenPos.xy * 0.5 + 0.5); // Map [-1, 1] to [0, 1]
//         lightScreenPos.xy *= vec2(1366.0, 768.0); // Replace with actual screen dimensions

//         // Calculate the distance between the current fragment position and the light position
//         float distance = length(vec2(gl_FragCoord.xy) - lightScreenPos.xy);

//         // If the fragment is close to a light, assign a bright color
//         if (distance < 10.0) {
//             debugColor = vec3(1.0, 1.0, 0.0); // Yellow for debugging lights
//         }
//     }

//     // Combine the base color and the debug overlay
//     FragColor = baseColor + vec4(debugColor, 0.0); // Add debug color without altering alpha
// }