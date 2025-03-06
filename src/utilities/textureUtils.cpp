#include "textureUtils.h"

/**
 * @brief Simple noise texture generator (vector of RGBAs). Used to debug u,v mapping.
 * 
 * @param width how many pixels wide
 * @param height how many pixels tall
 * @return std::vector<unsigned char> vector of RGBA values. 
 */
std::vector<unsigned char> generateNoiseTextureRGBA(int width, int height) {
    std::vector<unsigned char> noiseTexture(width * height * 4);

    std::srand(std::time(nullptr));  // Seed random generator

    for (int i = 0; i < width * height * 4; i += 4) {
        noiseTexture[i + 0] = std::rand() % 256; // R
        noiseTexture[i + 1] = std::rand() % 256; // G
        noiseTexture[i + 2] = std::rand() % 256; // B
        noiseTexture[i + 3] = 255;               // A (fully opaque)
    }

    return noiseTexture;
}

/**
 * @brief Create a Texture object from a PNGImage
 * 
 * @param image 
 * @return GLuint - the textureID
 */
GLuint createTexture(const PNGImage& image) {
    //GLuint textureID;
    unsigned int textureID;

    // Similar syntax and idea to making our VBOs and such...
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Copy raw image data to GPU (I think thats what this does)
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, 
        image.width, image.height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, image.pixels.data()
    );

    // Automatically generate a MipMap for our texture (wow!)
    // Acting on bound texture, so no extra param needed...
    glGenerateMipmap(GL_TEXTURE_2D);

    // Configure sampling for the texture...
    // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Texels smaller... (This is just pure interpolation - no mipmap)
    // glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // MipMap settings : X_MIPMAP_Y   X -> interpolation between mipmaps,  Y -> interpolation for sampling the mipmap itself.
    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST); // Apply mipmap as texel smaller sampling thingy like this :D

    glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Interpolate when texel larger than pixels.



    return textureID;
};