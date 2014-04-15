#ifndef test_int_tex_h
#define test_int_tex_h

#include <common.h>

/**
    Test writing to an integer texture by:
    Reading back the value that were just written to it
    Checking if the currently bound texture has integer format
    Writing to a FBO and reading back the values
*/
namespace ogle {
class Test_Integer_Texture
{
public:
    void init(const std::string& dataDirectory);
    void check_int_textur_bound();
    void read_back_texture();
    void write_read_to_fbo();
    void shutdown();
private:
    struct TexturePod
    {
        unsigned int width;
        unsigned int height;
        unsigned int componentCount;
        GLint internalFormat;
        GLenum format;
        GLenum type;
        GLenum target;
    } TextureInfo;

    GLuint TextureName;
    Framebuffer FBO;
    FullscreenQuad Quad;
    ShaderProgram Program;
};
}
#endif //test_int_tex_h