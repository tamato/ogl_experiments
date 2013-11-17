#ifndef common_gl_helpers
#define common_gl_helpers

#define GLEW_NO_GLU
#include <GL/glew.h>

namespace ogle
{
    class TextureObject
    {
    public:
        TextureObject();
        void initialize();
        void initialize(GLenum);
        void setWrapMode(GLenum);

    private:
        GLenum  Target;
        GLint   Level;
        GLint   InternalFormat;
        GLsizei Width;
        GLsizei Height;
        GLint   Border;
        GLenum  Format;
        GLenum  Type;
    };

    GLuint createTexture();
    GLuint createFramebuffer();
    GLuint createShader();
    void   initGLEW();
    void   setDataDir(int argc, char *argv[]);
}

#endif // common_gl_helpers
