#ifndef ogle_framebuffer
#define ogle_framebuffer

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>

#include "texture.h"

namespace ogle
{
    struct RenderTarget
    {
        RenderTarget();

        void init(bool clearColor, bool clearDepth, bool clearStencil);
        void attachColor(const std::vector<Texture>& textures);
        void attachColor(const Texture& texture);
        void attachDepth(const Texture& texture);
        void attachStencil(const Texture& texture);
        void checkStatus();
        void bind();
        void unbind();
        void clear();
        void shutdown();

        unsigned int  FramebufferName;

        glm::vec4 ClearColor;
        float ClearDepth;
        float ClearStencil;

        bool UseClearColor;
        bool UseClearDepth;
        bool UseClearStencil;

        int NumColorBuffers;
    };
}
#endif // ogle_framebuffer