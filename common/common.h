#ifndef common_gl_helpers
#define common_gl_helpers

#define GLEW_NO_GLU
#include <GL/glew.h>

#include <string>
#include <vector>
#include <map>

namespace ogle
{
    struct Framebuffer
    {
        std::vector<GLuint>  TextureNames;
        GLuint  FramebufferName;
        GLenum  Target;
        GLuint  ComponentCount;
        GLint   InternalFormat;
        GLsizei Width;
        GLsizei Height;
        GLenum  Format;
        GLenum  Type;
    };

    GLuint initTexture(GLenum target, GLint internalFormat, GLuint componentCount, GLsizei width, GLsizei height, GLenum format, GLenum type);
    // framebuffer releated
    void initFramebuffer(Framebuffer& framebuffer);
    void checkFramebufferStatus();

    // shader related
    void   checkShaderLinkage( const GLuint& program);
    GLuint createShader(GLenum type, const std::string& filename);
    GLuint createProgram( const std::map<GLuint, std::string>& shaders );

    // misc
    void   initGLEW();
    void   setDataDir(int argc, char *argv[]);
}

#endif // common_gl_helpers
