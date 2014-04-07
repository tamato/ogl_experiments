#ifndef ogle_framebuffer
#define ogle_framebuffer

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <map>

namespace ogle
{
    struct iaFramebuffer
    {
        iaFramebuffer();

        void init();
        void checkStatus();
        void bind();
        void unbind();
        void clear();


        GLuint  FramebufferName;
        GLuint  ComponentCount;
        GLint   InternalFormat;
        GLsizei Width;
        GLsizei Height;
        GLenum  Target;
        GLenum  Format;
        GLenum  Type;

        std::vector<GLuint>  TextureNames;
        GLuint  DepthName;
        GLuint  StencilName;

        glm::vec4 ClearColor;
        float ClearDepth;
        float ClearStencil;
    };
}
#endif // ogle_framebuffer