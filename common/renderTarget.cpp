#include "renderTarget.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace ogle;

RenderTarget::RenderTarget()
    : FramebufferName(0)
    , ClearColor(0)
    , ClearDepth(0)
    , ClearStencil(0)
    , UseClearColor(false)
    , UseClearDepth(false)
    , UseClearStencil(false)
    , NumColorBuffers(0)
{

}

void RenderTarget::init(bool clearColor, bool clearDepth, bool clearStencil)
{
    UseClearColor = clearColor;
    UseClearDepth = clearDepth;
    UseClearStencil = clearStencil;
    glGenFramebuffers(1, &FramebufferName);
}

void RenderTarget::attachColor(const std::vector<Texture>& textures)
{
    NumColorBuffers = (int)textures.size();
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    for (size_t i=0; i<NumColorBuffers; ++i)
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, textures[i].Target, textures[i].Name, 0);

    // check for completeness
    checkStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::attachColor(const Texture& texture)
{
    NumColorBuffers = 1;
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture.Target, texture.Name, 0);

    // check for completeness
    checkStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::attachDepth(const Texture& texture)
{

}

void RenderTarget::attachStencil(const Texture& texture)
{

}

void RenderTarget::checkStatus()
{
    GLenum result = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if (result != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "FBO incomplete, error: " << std::endl;
        switch (result)
        {
            /*********************************************************************
              These values were found from:
                http://www.opengl.org/wiki/GLAPI/glCheckFramebufferStatus
            *********************************************************************/
            case GL_FRAMEBUFFER_UNDEFINED:
                std::cout << "\tGL_FRAMEBUFFER_UNDEFINED\n";
                std::cout << "\t-target- is the default framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n";
                std::cout << "\tthe framebuffer attachment is incomplete";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n";
                std::cout << "\tthere are no images attached to the framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n";
                std::cout << "\tone of the color attaches has an object type of GL_NONE";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n";
                std::cout << "\tGL_READ_BUFFER attached object type of GL_NONE";
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                std::cout << "\tGL_FRAMEBUFFER_UNSUPPORTED\n";
                std::cout << "\tinternal formats conflict with implementation-dependent restrictions";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n";
                std::cout << "\tis also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS"
                     << "\tis not the same for all attached textures; or, if the attached images"
                     << "\tare a mix of renderbuffers and textures, the value of"
                     << "\tGL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n";
                std::cout << "\ta framebuffer attachment is layered, and a populated attachment is not layered,"
                     << "\tor if all populated color attachments are not from textures of the same target.";
                break;
            default:
                std::cout << "\tUnknown error occured.";
        }
        std::cout << std::endl;
        assert(0);
    }
}

void RenderTarget::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    unsigned int bitmask = 0;
    if (UseClearColor){
        // NumColorBuffers
        // glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glClearColor( ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3] );
        bitmask |= GL_COLOR_BUFFER_BIT; 
    }

    if (bitmask != 0)
        glClear( bitmask );
}

void RenderTarget::unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::clear()
{

}

void RenderTarget::shutdown()
{
    glDeleteFramebuffers(1, &FramebufferName);
    FramebufferName = 0;
}
