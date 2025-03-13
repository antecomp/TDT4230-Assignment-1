#version 450 core

//#define NUM_LIGHT_SOURCES 3
#define NUM_LIGHT_SOURCES 3

in layout(location = 0) vec3 normal;
in layout(location = 1) vec2 textureCoordinates;

in layout(location = 3) mat3 TBN;

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

float la = 0.003;
float lb = 0.001;
float lc = 0.002;

uniform bool is2D;
uniform bool hasNormalMappedGeom;


layout(binding = 0) uniform sampler2D textSampler;

layout(binding = 1) uniform sampler2D diffuseSampler;
layout(binding = 2) uniform sampler2D normalMapSampler;

void main()
{

    // I.e used for our text layer.
    if(is2D) {
        vec4 sampled = texture(textSampler, textureCoordinates);
        color = vec4(1.0, 0.8, 0.2, 1.0); 
        color = sampled;

        color = texture(textSampler, textureCoordinates);
        //color = vec4(textureCoordinates, 0.0, 1.0);
        //color = vec4(textureCoordinates.x, 0.0, 0.0, 1.0); // Red = U coordinate

        // A bunch of testing stuff becuase I had my UV mapping wrong and was debugging it
        // keeping because it's a helpful reference of *how* to debug stuff..

        // Manually get a UV just based on the coordinates - should show top of texture scaled
        //vec2 screenUV = gl_FragCoord.xy / vec2(128.0, 128.0); // Scale to match the noise texture size
        //vec2 screenUV = gl_FragCoord.xy / textureSize(textSampler, 0);
        //color = texture(textSampler, screenUV);

        // Visualize U,V Coordinates...
        //color = vec4(textureCoordinates, 0.0, 1.0); // Both coordinates
        //color = vec4(textureCoordinates.x, 0.0, 0.0, 1.0); // Red = U coordinate (this is the one I messed up lol)

        return;
    }



        vec3 normalToUse = normal;
        vec3 baseDiffuseToUse = vec3(1.0, 1.0, 1.0);

        if (hasNormalMappedGeom) {
            vec3 diffuseColor = texture(diffuseSampler, textureCoordinates).rgb;
            vec3 normalMapColor = texture(normalMapSampler, textureCoordinates).rgb;

            // Swap to [-1,1]
            vec3 normalMapped = texture(normalMapSampler, textureCoordinates).rgb;
            normalMapped = normalMapped * 2.0 - 1.0;

            // (test) All work
            //color = vec4(normalMapColor, 1.0);
            //color = vec4(diffuseColor, 1.0);
            //color = vec4(normalMapped, 1.0);
            // color = vec4(TBN * normalMapped, 1.0);

            //normalToUse = normalMapped;
            normalToUse = TBN * normalMapped;
            baseDiffuseToUse = diffuseColor;
        }

            // color = vec4(textureCoordinates, 0.0, 1.0);
            // return;







    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    for(int i = 0; i < NUM_LIGHT_SOURCES; ++i) {
        vec3 lightDistance = lightSources[i].position - fragWSPosition;
        vec3 lightDir = normalize(lightDistance);
        float d = length(lightDistance);
        float L = 1.0 / (la + d * lb + d * d * lc);

        // Diffuse
        vec3 diffuseColour = lightSources[i].color;
        float diff = max(dot(normalToUse, lightDir), 0.0); 
        totalDiffuse += L * diff * diffuseColour;

        // Specular Reflect
        vec3 reflectDir = reflect(-lightDir, normalToUse); // For ease we're directly reflecting back towards light
        vec3 viewDir = normalize(u_cameraPosition - fragWSPosition); // View direction toward the camera

        // Specular intensity = reflected vector dot view (surface 2 eye)
        // If the specular factor is negative, you should set it to 0.
        // 8.0 is the shininess factor (hard coded, will make uniform l8r).
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);


        vec3 specularColour = lightSources[i].color * 1.5;
        totalSpecular += L * spec * specularColour;
    }

    vec3 finalColour = baseDiffuseToUse * (ambientIntensity + totalDiffuse + totalSpecular + dither(gl_FragCoord.xy));

    color = vec4(finalColour, 1.0);

}