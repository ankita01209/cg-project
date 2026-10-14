#include "render/Texture.h"
#include <iostream>
#include <utility>
#include <GLFW/glfw3.h> // for timing
#include "../libs/stb/stb_image.h" // Relative to src/render/

namespace render {

Texture::Texture() {}

Texture::~Texture()
{
    if (m_id != 0) glDeleteTextures(1, &m_id);
}

Texture::Texture(Texture&& other) noexcept
    : m_id(other.m_id), m_loaded(other.m_loaded)
{
    other.m_id = 0;
    other.m_loaded = false;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        if (m_id != 0) glDeleteTextures(1, &m_id);
        m_id = other.m_id;
        m_loaded = other.m_loaded;
        other.m_id = 0;
        other.m_loaded = false;
    }
    return *this;
}

bool Texture::load(const std::string& path)
{
    double startTime = glfwGetTime();

    // AUDIT FINDING: UV Mapping orientation
    // Sphere.cpp parameterises 'v=0' at the North Pole (phi=0).
    // Usually, image data arrays flow top-to-bottom. If we feed this data into
    // OpenGL's bottom-up coordinate system natively, the first row (the top of the image)
    // maps to GL's v=0. Since our v=0 IS the top (North Pole), this automatically aligns!
    // No vertical flipping is physically required here.
    stbi_set_flip_vertically_on_load(false);

    int width, height, channels;
    // Force 4 channels (RGBA) for consistent GL internal formats
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (!data)
    {
        std::cerr << "[Texture] Failed to load texture at path: " << path << "\n";
        return false;
    }

    // Check hardware bounds
    GLint maxTexSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (width > maxTexSize || height > maxTexSize)
    {
        std::cerr << "[Texture] Image exceeds GL_MAX_TEXTURE_SIZE (" << maxTexSize
                  << ") at path: " << path << "\n";
        stbi_image_free(data);
        return false;
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    // Byte alignment for robustness (especially true if not coercing to 4 channels)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Wrap policies for a spherical equirectangular mapping
    // U (longitude) flows seamlessly across the seam => REPEAT
    // V (latitude) poles pinch => CLAMP_TO_EDGE limits wraparound smearing artifacts
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Filtering (Trilinear mipmapping)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);

    m_loaded = true;

    double loadTime = (glfwGetTime() - startTime) * 1000.0;
    std::cout << "[Texture] Loaded " << width << "x" << height
              << " texture from " << path << " (" << loadTime << " ms)\n";

    return true;
}

void Texture::bind(GLuint textureUnit) const
{
    if (m_loaded)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_id);
    }
}

} // namespace render
