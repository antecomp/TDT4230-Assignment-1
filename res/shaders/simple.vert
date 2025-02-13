#version 450 core

in layout(location = 0) vec3 position;
in layout(location = 1) vec3 normal_in;
in layout(location = 2) vec2 textureCoordinates_in;

in layout(location = 3) vec3 tangent_in;
in layout(location = 4) vec3 bitangent_in;

uniform layout(location = 7) mat3 normalTransform;

uniform layout(location = 3) mat4 MVP;
uniform layout(location = 4) mat4 M;

out layout(location = 0) vec3 normal_out;
out layout(location = 1) vec2 textureCoordinates_out;

out vec3 fragWSPosition; // Worlspace position (no view/projection transform)

out layout(location = 3) mat3 TBN;

uniform bool is2D;

uniform mat4 Ortho;

void main()
{
    if(is2D) {
        gl_Position = Ortho * M* vec4(position, 1.0f);
        textureCoordinates_out = textureCoordinates_in;
        return;
    }

    // TBN Calculation...
    vec3 T = normalize(normalTransform * tangent_in);
    vec3 B = normalize(normalTransform * bitangent_in);
    vec3 N = normalize(normalTransform * normal_in);
    TBN = mat3(T, B, N);


    normal_out = normalize(normalTransform * normal_in);
    //normal_out = normal_in;
    textureCoordinates_out = textureCoordinates_in;
    fragWSPosition = (M * vec4(position, 1.0f)).xyz;
    gl_Position = MVP * vec4(position, 1.0f); 
}
