#ifndef common_gl_helpers
#define common_gl_helpers

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>         // glm::value_ptr
#include <glm/gtx/string_cast.hpp>      // glm::to_string

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <map>

namespace ogle
{
    struct Framebuffer
    {
        Framebuffer();
        std::vector<GLuint>  TextureNames;
        GLuint  FramebufferName;
        GLenum  Target;
        GLuint  ComponentCount;
        GLint   InternalFormat;
        GLsizei Width;
        GLsizei Height;
        GLenum  Format;
        GLenum  Type;

        void shutdown();
    };

    struct ShaderProgram
    {
        GLuint ProgramName;
        std::map<std::string, GLint> Uniforms;

        ShaderProgram();
        ~ShaderProgram();
        void init( const std::map<GLuint, std::string>& shaders );
        void bind();
        void collectUniforms();
        void shutdown();

    private:
        GLuint createShader(GLenum type, const std::string& filename);
        void   checkShaderLinkage( const GLuint& program);
    };

    struct FullscreenQuad
    {
        explicit FullscreenQuad();
        ~FullscreenQuad();

        void init();
        void render();
        void shutdown();

        const GLsizei VertCount;
        const GLsizei ByteCount;
        const std::vector<glm::vec2> Verts;
        GLuint VAO_Name;
        GLuint BufferName;
    };

    GLuint initTexture(GLenum target, GLint internalFormat, GLuint componentCount, GLsizei width, GLsizei height, GLenum format, GLenum type);
    // framebuffer releated
    void initFramebuffer(Framebuffer& framebuffer);
    void checkFramebufferStatus();

    // setup
    typedef void (APIENTRY *key_call_back)(GLFWwindow*, int, int, int, int);
    typedef void (APIENTRY *error_call_back)(int, const char*);
    GLFWwindow*  initGLFW(int major_version,
                    int minor_version,
                    bool debug_ctx,
                    int window_width,
                    int window_height,
                    std::string window_name,
                    key_call_back key_callback,
                    error_call_back err_callback);
    void   initGLEW();
    void   setDataDir(int argc, char *argv[]);
}

#endif // common_gl_helpers
