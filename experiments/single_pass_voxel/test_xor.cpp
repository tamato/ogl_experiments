#include "test_xor.h"

#include <iostream>
#include <map>

using namespace ogle;

TestXOR::TestXOR()
{

}

TestXOR::~TestXOR()
{

}

void TestXOR::init(const std::string& base_dir)
{
    // create the quad
    {
        Quad.init();
    }

    // create the shaders for the quad
    {
        std::map<GLuint, std::string> shaders;
        shaders[GL_VERTEX_SHADER] = base_dir + "int_texture_test.vert";
        shaders[GL_FRAGMENT_SHADER] = base_dir + "int_texture_test.frag";
        Program.ProgramName = createProgram(shaders);
        Program.collectUniforms();
    }

    // framebuffer for the quad to draw too
    {
        Framebuffer.TextureNames.resize(1);
        Framebuffer.Target = GL_TEXTURE_2D;
        Framebuffer.ComponentCount = 1;
        Framebuffer.InternalFormat = GL_R32UI;
        Framebuffer.Width = 8;
        Framebuffer.Height = 1;
        Framebuffer.Format = GL_RED_INTEGER;
        Framebuffer.Type = GL_UNSIGNED_INT;
        initFramebuffer(Framebuffer);
    }

    // texture to be used by the quad
    {
        TextureName = initTexture(
            Framebuffer.Target,
            Framebuffer.InternalFormat,
            Framebuffer.ComponentCount,
            Framebuffer.Width,
            Framebuffer.Height,
            Framebuffer.Format,
            Framebuffer.Type);

        glBindTexture(Framebuffer.Target, TextureName);

        // init the texture to a specfic value to test
        unsigned int size = Framebuffer.Width * Framebuffer.Height * Framebuffer.ComponentCount;
        unsigned int *data = new unsigned int[size];
        data[0] = 0xAAAAAAAAu;  // 0101... 2863311530
        data[1] = 0xFFFFFFFFu;  // 1111... 4294967295
        data[2] = 0x0u;         // 0000... 0
        data[3] = 0xCCCCCCCCu;  // 1100... 3435973836
        data[4] = 0x33333333u;  // 0011... 858993459
        data[5] = 0x88888888u;  // 1000... 2290649224
        data[6] = 0x11111111u;  // 0001... 286331153
        data[7] = 0x44444444u;  // 0100... 1145324612
        glTexSubImage2D(
            Framebuffer.Target, 0,
            0, 0,
            Framebuffer.Width,
            Framebuffer.Height,
            Framebuffer.Format,
            Framebuffer.Type,
            data
        );
        delete [] data;
        glBindTexture(Framebuffer.Target, 0);
    }
}

void TestXOR::run()
{
    // clear framebuffer to test value
    {
        glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FramebufferName);
        glViewport(0, 0, Framebuffer.Width, Framebuffer.Height);

        GLuint clear_color[4] = {0,0,0,0};
        glClearBufferuiv(GL_COLOR, 0, clear_color);
    }

    // enables...
    {
        glEnable(GL_COLOR_LOGIC_OP);    // disables blending
        glLogicOp(GL_XOR);
    }

    // set shader and texture
    {
        glUseProgram(Program.ProgramName);
        glBindTexture(Framebuffer.Target, TextureName);
    }

    // draw quad
    {
        Quad.render();
    }

    // undo opengl state
    {
        glBindVertexArray(0);
        glBindTexture(Framebuffer.Target, 0);
        glUseProgram(0);

        glDisable(GL_COLOR_LOGIC_OP);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // read back the values
    {
        unsigned int size = Framebuffer.Width * Framebuffer.Height * Framebuffer.ComponentCount;
        unsigned int *data = new unsigned int[size];
        glBindTexture(Framebuffer.Target, Framebuffer.TextureNames[0]);
        glGetTexImage(Framebuffer.Target, 0,
            Framebuffer.Format,
            Framebuffer.Type,
            (GLvoid*)data
            );

        std::cout   << std::boolalpha
                    << "TestXOR::run:"
                    << "\n\t0 idx: " << (data[0] == 0xAAAAAAAAu) << " " << data[0]
                    << "\n\t1 idx: " << (data[1] == 0xFFFFFFFFu) << " " << data[1]
                    << "\n\t2 idx: " << (data[2] == 0x0u)        << " " << data[2]
                    << "\n\t3 idx: " << (data[3] == 0xCCCCCCCCu) << " " << data[3]
                    << "\n\t4 idx: " << (data[4] == 0x33333333u) << " " << data[4]
                    << "\n\t5 idx: " << (data[5] == 0x88888888u) << " " << data[5]
                    << "\n\t6 idx: " << (data[6] == 0x11111111u) << " " << data[6]
                    << "\n\t7 idx: " << (data[7] == 0x44444444u) << " " << data[7]
                    << std::endl;
        delete [] data;
    }
}

void TestXOR::shutdown()
{

}
