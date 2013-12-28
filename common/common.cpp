#include "common.h"

#include <cassert>
#include <fstream>
#include <iostream>

namespace {

}

namespace ogle {

    GLuint initTexture(GLenum target, GLint internalFormat, GLuint componentCount, GLsizei width, GLsizei height, GLenum format, GLenum type)
    {
        GLuint textureName;
        glGenTextures(1, &textureName);
        glBindTexture(target, textureName);

        unsigned int size = width * height * componentCount;
        unsigned int *data = new unsigned int[size];
        for (int i=0; i<size; i+=componentCount)
            data[i] = 0;

        // Nvidia has a bug that if these were intergal textures
        // GL_LINEAR cannot be used and must be GL_NEAREST
        glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexImage2D(
            target, 0,
            internalFormat,
            width,
            height,
            0,
            format,
            type,
            data
        );

        if (glGetError() != GL_NONE) assert(0);

        glBindTexture(target, 0);
        delete [] data;
        return textureName;
    }


    void initFramebuffer(Framebuffer& framebuffer)
    {
        for (auto& texture : framebuffer.TextureNames) {
            texture = initTexture(
                framebuffer.Target,
                framebuffer.InternalFormat,
                framebuffer.ComponentCount,
                framebuffer.Width,
                framebuffer.Height,
                framebuffer.Format,
                framebuffer.Type
                );
        }

        glGenFramebuffers(1, &framebuffer.FramebufferName);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.FramebufferName);
        for (size_t i=0; i<framebuffer.TextureNames.size(); ++i)
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, framebuffer.Target, framebuffer.TextureNames[i], 0);

        // check for completeness
        ogle::checkFramebufferStatus();
    }

    void checkFramebufferStatus()
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

    void checkShaderLinkage( const GLuint& program)
    {
        int status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            const int maxLen = 1000;
            int len;
            char errorBuffer[maxLen]{0};
            glGetProgramInfoLog(program, maxLen, &len, errorBuffer);
            std::cerr   << "Shader Linked with erros:\n"
                        << errorBuffer << std::endl;
            assert(0);
        }
    }

    GLuint createShader(GLenum type, const std::string& filename)
    {
        GLuint shader = glCreateShader(type);

        // load up the file
        std::ifstream inf(filename);
        if (!inf.is_open()) {
            std::cerr << "Failed to open " << filename << std::endl;
            assert(0);
        }

        std::string source;
        inf.seekg(0, std::ios::end);
        source.resize(inf.tellg());
        inf.seekg(0, std::ios::beg);
        inf.read(&source[0], source.size());
        inf.close();

        const char *c_str = source.c_str();
        glShaderSource(shader, 1, &c_str, NULL);
        glCompileShader(shader);

        int status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
        {
            const int maxLen = 1000;
            int len;
            char errorBuffer[maxLen]{0};
            glGetShaderInfoLog(shader, maxLen, &len, errorBuffer);
            std::cerr   << "Shader Compile with erros:\n"
                        << errorBuffer << std::endl;
            assert(0);
        }

        return shader;
    }

    GLuint createProgram( const std::map<GLuint, std::string>& shaders )
    {
        std::vector<GLuint> shader_list;
        for (const auto& kv: shaders)
            shader_list.push_back( createShader( kv.first, kv.second ) );

        GLuint program = glCreateProgram();
        for (const auto& shader : shader_list)
            glAttachShader(program, shader);

        glLinkProgram(program);

        for (const auto& shader : shader_list)
            glDeleteShader(shader);

        checkShaderLinkage(program);
        return program;
    }
}