#pragma once

#include <GL/glew.h>
#include <cstddef>
#include <string>

GLuint loadTexture(const std::string& path);
GLuint loadTextureFromMemory(const unsigned char* bytes, std::size_t length);
