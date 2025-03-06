#include "glad/glad.h"
#include "utilities/imageLoader.hpp"
#include <vector>
#include <cstdlib> // For rand()
#include <ctime>   // For seeding rand()

std::vector<unsigned char> generateNoiseTextureRGBA(int, int);

GLuint createTexture(const PNGImage&);