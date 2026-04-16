#include "texture.h"

#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {
GLuint createTexture2D(unsigned char* pixels, int width, int height, int channels)
{
    if (!pixels || width <= 0 || height <= 0) {
        return 0;
    }

    GLenum format = GL_RGB;
    if (channels == 1) format = GL_RED;
    if (channels == 3) format = GL_RGB;
    if (channels == 4) format = GL_RGBA;

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Anisotropic filtering — critical for ground textures viewed at glancing angles
    GLfloat maxAniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
    if (maxAniso > 1.0f) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}
}  // namespace

GLuint loadTexture(const std::string& path)
{
    stbi_set_flip_vertically_on_load(true);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return 0;
    }

    const GLuint tex = createTexture2D(data, width, height, channels);
    stbi_image_free(data);
    return tex;
}

GLuint loadTextureFromMemory(const unsigned char* bytes, std::size_t length)
{
    if (!bytes || length == 0) {
        return 0;
    }

    stbi_set_flip_vertically_on_load(true);
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load_from_memory(bytes, static_cast<int>(length), &width, &height, &channels, 0);
    if (!data) {
        return 0;
    }

    const GLuint tex = createTexture2D(data, width, height, channels);
    stbi_image_free(data);
    return tex;
}
