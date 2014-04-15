#include "test_integer_texture.h"

#include <iostream>

using namespace ogle;

void Test_Integer_Texture::init(const std::string& dataDirectory)
{
    TextureInfo.width = 1;
    TextureInfo.height = 1;
    TextureInfo.componentCount = 4;
    TextureInfo.internalFormat = GL_RGBA32UI;
    TextureInfo.format = GL_RGBA_INTEGER;
    TextureInfo.type = GL_UNSIGNED_INT;
    TextureInfo.target = GL_TEXTURE_2D;

    // get texture setup
    {
        glGenTextures(1, &TextureName);
        glBindTexture(TextureInfo.target, TextureName);

        unsigned int size = TextureInfo.width * TextureInfo.height * TextureInfo.componentCount;
        unsigned int *data = new unsigned int[size];
        for (int i=0; i<size; i+=TextureInfo.componentCount){
            data[i+0] = 100;
            data[i+1] = 1000;
            data[i+2] = 999;
            data[i+3] = 65000;
        }

        // Nvidia has a bug that if these were intergal textures
        // GL_LINEAR cannot be used and must be GL_NEAREST
        glTexParameteri( TextureInfo.target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( TextureInfo.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( TextureInfo.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( TextureInfo.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexImage2D(
            TextureInfo.target, 0,
            TextureInfo.internalFormat,
            TextureInfo.width,
            TextureInfo.height,
            0,
            TextureInfo.format,
            TextureInfo.type,
            data
        );

        if (glGetError() != GL_NONE) assert(0);
        glBindTexture(TextureInfo.target, 0);
    }

    // fbo set up
    {
        FBO.InternalFormat = TextureInfo.internalFormat;
        FBO.Target = TextureInfo.target;
        FBO.ComponentCount = TextureInfo.componentCount;
        FBO.Width = TextureInfo.width;
        FBO.Height = TextureInfo.height;
        FBO.Format = TextureInfo.format;
        FBO.Type = TextureInfo.type;
        FBO.TextureNames.resize(1);
        ogle::initFramebuffer(FBO);
    }

    // quads turn
    {
        Quad.init();
    }

    // and shaders
    {
        std::map<GLuint, std::string> shaders;
        shaders[GL_VERTEX_SHADER] = dataDirectory + "int_texture_test.vert";
        shaders[GL_FRAGMENT_SHADER] = dataDirectory + "int_texture_test.frag";
        Program.ProgramName = createProgram(shaders);
        Program.collectUniforms();
    }
    glFinish();
}

void Test_Integer_Texture::check_int_textur_bound()
{
    glBindTexture(TextureInfo.target, TextureName);
    GLboolean b;
    glGetBooleanv(GL_RGBA_INTEGER_MODE_EXT, &b);
    std::cout   << "Test_Integer_Texture::check_int_textur_bound"
                << "\n\tBound texture is integer: " << std::boolalpha << (b == GL_TRUE) << std::endl;
    glBindTexture(TextureInfo.target, 0);
}

void Test_Integer_Texture::read_back_texture()
{
    glBindTexture(TextureInfo.target, TextureName);
    unsigned int size = TextureInfo.width * TextureInfo.height * TextureInfo.componentCount;
    unsigned int *data = new unsigned int[size];
    glGetTexImage(TextureInfo.target, 0, TextureInfo.format, TextureInfo.type, (GLvoid*)data);

    std::cout   << std::boolalpha
                << "Test_Integer_Texture::read_back_texture:"
                << "\n\tR match: " << (data[0] == 100)
                << "\n\tG match: " << (data[1] == 1000)
                << "\n\tB match: " << (data[2] == 999)
                << "\n\tA match: " << (data[3] == 65000)
                << std::endl;
    glBindTexture(TextureInfo.target, 0);
}

void Test_Integer_Texture::write_read_to_fbo()
{
    // write to fbo
    {
        glDisable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, FBO.FramebufferName);
        glViewport( 0, 0, FBO.Width, FBO.Height);

        GLuint clear_color[4] = {0,0,0,0};
        glClearBufferuiv(GL_COLOR, 0, clear_color);

        glUseProgram(Program.ProgramName);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(TextureInfo.target, TextureName);

        Quad.render();
        glFinish();
    }

    // read data back from it
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(TextureInfo.target, FBO.TextureNames[0]);
        unsigned int size = FBO.Width * FBO.Height * FBO.ComponentCount;
        unsigned int *data = new unsigned int[size];
        glGetTexImage(FBO.Target, 0, FBO.Format, FBO.Type, (GLvoid*)data);
        std::cout   << std::boolalpha
                    << "Test_Integer_Texture::write_read_to_fbo:"
                    << "\n\tR match: " << (data[0] == 100)      << " " << data[0]
                    << "\n\tG match: " << (data[1] == 1000)     << " " << data[1]
                    << "\n\tB match: " << (data[2] == 999)      << " " << data[2]
                    << "\n\tA match: " << (data[3] == 65000)    << " " << data[3]
                    << std::endl;
        glBindTexture(TextureInfo.target, 0);
    }
}

void Test_Integer_Texture::shutdown()
{

}