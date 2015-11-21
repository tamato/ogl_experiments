#ifndef ogle_texture 
#define ogle_texture

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
    struct Texture
    {
        int InternalFormat;
        
        unsigned int Name;
        unsigned int ComponentCount;
        unsigned int Width;
        unsigned int Height;
        unsigned int Target;
        unsigned int Format;
        unsigned int Type;

        unsigned int MinFilter;
        unsigned int MagFilter;
        unsigned int WrapS;
        unsigned int WrapT;

        Texture();
        ~Texture();
        void create();
        void create(unsigned int width, unsigned int height);
        void bindAsTexture(int stage);
        void unbindTexture(int stage);
        void bindAsImage(int stage, unsigned int access);
        void clear();
        void shutdown();
    };
}

#endif