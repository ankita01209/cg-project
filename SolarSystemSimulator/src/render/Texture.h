#pragma once
// =============================================================================
// Texture.h â€” OpenGL texture resource wrapper
// Responisibilities:
//   â€¢ RAII encapsulation of a GL_TEXTURE_2D
//   â€¢ Image parsing using stb_image
//   â€¢ Mipmap logic and filtering parameters
// =============================================================================

#include <glad/glad.h>
#include <string>

namespace render {

class Texture
{
public:
    Texture();
    ~Texture();

    // Prevent copying to maintain strict RAII over the GPU resource
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Movement safely passes ownership
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /// Loads an image from the absolute path.
    /// Gracefully fails and returns false if the texture cannot be parsed.
    bool load(const std::string& path);

    /// Activates the texture in the current OpenGL context's specific slot.
    void bind(GLuint textureUnit = 0) const;

    /// Validation hook for shader branching.
    bool hasTexture() const { return m_loaded && m_id != 0; }

private:
    GLuint m_id = 0;
    bool   m_loaded = false;
};

} // namespace render
