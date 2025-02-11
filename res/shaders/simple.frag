#version 430 core

#define NUM_LIGHT_SOURCES 3

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

out vec4 color;

float rand(vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453); }
float dither(vec2 uv) { return (rand(uv)*2.0-1.0) / 256.0; }

vec3 ambientIntensity = vec3(0.15, 0.15, 0.15);

struct LightSource {
    vec3 position;
    vec3 color;
};

uniform LightSource lightSources[NUM_LIGHT_SOURCES];
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

uniform bool performPhong;

void main()
{

    // I.e used for our text layer.
    if(!performPhong) {
        color = vec4(1.0, 0.8, 0.2, 1.0); 
        return;
    }

    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    for(int i = 0; i < NUM_LIGHT_SOURCES; ++i) {
        vec3 lightDistance = lightSources[i].position - fragWSPosition;
        vec3 lightDir = normalize(lightDistance);
        float d = length(lightDistance);
        float L = 1.0 / (la + d * lb + d * d * lc);

        // For smooth shadows we should instead scale some factor
        // We can just scale L directly to make our lives easier...
        float innerRadius = ballRadius;         // Hard shadow cutoff
        float outerRadius = ballRadius * 1.5;   // Start of soft shadow transition
        vec3 toLight = lightSources[i].position - fragWSPosition;
        float rejectLength = length(reject(toBall, toLight));

        if (
            length(toBall) < length(toLight) // Ball is between light and fragment
            && rejectLength < outerRadius // Fragment is within extended occlusion width
            && dot(toBall, toLight) > 0.0 // Ball is "blocking" light (not behind it)
        ) {
            // Thanks google: https://registry.khronos.org/OpenGL-Refpages/gl4/html/smoothstep.xhtml
            float shadowFactor = smoothstep(innerRadius, outerRadius, rejectLength);
            L *= shadowFactor;
        }

        // Diffuse
        vec3 diffuseColour = lightSources[i].color;
        float diff = max(dot(normal, lightDir), 0.0); 
        totalDiffuse += L * diff * diffuseColour;

        // Specular Reflect
        vec3 reflectDir = reflect(-lightDir, normal); // For ease we're directly reflecting back towards light
        vec3 viewDir = normalize(u_cameraPosition - fragWSPosition); // View direction toward the camera

        // Specular intensity = reflected vector dot view (surface 2 eye)
        // If the specular factor is negative, you should set it to 0.
        // 8.0 is the shininess factor (hard coded, will make uniform l8r).
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);


        vec3 specularColour = lightSources[i].color * 1.5;
        totalSpecular += L * spec * specularColour;
    }

    vec3 finalColour = ambientIntensity + totalDiffuse + totalSpecular + dither(gl_FragCoord.xy);

    color = vec4(finalColour, 1.0);

}