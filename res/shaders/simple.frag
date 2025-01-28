#version 430 core

#define NUM_LIGHT_SOURCES 3

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }


vec3 ambientIntensity = vec3(0.15, 0.15, 0.15);
vec3 diffuseColour = vec3(0.4, 0.4, 0.4); // SHould be a uniform later :)
vec3 specularColour = vec3(0.7, 0.7, 0.7); // Specular reflection color (also make uniform later??)

uniform vec3 lightPositions[NUM_LIGHT_SOURCES];
uniform vec3 u_cameraPosition;

in vec3 fragWSPosition; 

uniform vec3 u_ballPosition;
float ballRadius = 3.0;
vec3 reject(vec3 from, vec3 onto) {
    return from - onto*dot(from, onto)/dot(onto, onto);
}

vec3 toBall = u_ballPosition - fragWSPosition;

float la = 0.003;
float lb = 0.001;
float lc = 0.002;

void main()
{
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    for(int i = 1; i < NUM_LIGHT_SOURCES; i++) {
        vec3 lightDistance = lightPositions[i] - fragWSPosition;
        vec3 lightDir = normalize(lightDistance);
        float d = length(lightDistance);
        float L = 1.0 / (la + d * lb + d * d * lc);


        // // Shadow Check
        vec3 toLight = lightPositions[i] - fragWSPosition;

       float rejectLength = length(reject(toBall, toLight));
        if (
            length(toBall) < length(toLight) // Ball is between light and fragment
            && rejectLength < ballRadius // Fragment is within occlusion width
        ) {
            continue;
        }






        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0); 
        totalDiffuse += L * diff * diffuseColour;

        // Specular Reflect
        vec3 reflectDir = reflect(-lightDir, normal); // For ease we're directly reflecting back towards light
        vec3 viewDir = normalize(u_cameraPosition - fragWSPosition); // View direction toward the camera

        // Specular intensity = reflected vector dot view (surface 2 eye)
        // If the specular factor is negative, you should set it to 0.
        // 8.0 is the shininess factor (hard coded, will make uniform l8r).
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);

        totalSpecular += L * spec * specularColour;
    }

    vec3 finalColour = ambientIntensity + totalDiffuse + totalSpecular + dither(gl_FragCoord.xy);

    color = vec4(finalColour, 1.0);

}