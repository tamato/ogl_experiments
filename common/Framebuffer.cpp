#include "Framebuffer.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace ogle;

iaFramebuffer::iaFramebuffer()
    : FramebufferName(0)
    , ComponentCount(0)
    , InternalFormat(0)
    , Width(0)
    , Height(0)
    , Target(0)
    , Format(0)
    , Type(0)
    , DepthName(0)
    , StencilName(0)
{

}

void iaFramebuffer::init()
{
    for (auto& texture : TextureNames) {
        // texture = initTexture(
        //     Target,
        //     InternalFormat,
        //     ComponentCount,
        //     Width,
        //     Height,
        //     Format,
        //     Type
        //     );
    }

    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    for (size_t i=0; i<TextureNames.size(); ++i)
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, Target, TextureNames[i], 0);

    // check for completeness
    checkStatus();
}

void iaFramebuffer::checkStatus()
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
