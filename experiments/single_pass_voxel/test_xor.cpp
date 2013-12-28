#include "test_xor.h"

#include "common.h"

#include <iostream>
#include <map>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

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
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
        glVertexAttribFormatNV(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));

        glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
        glEnableVertexAttribArray(0); // positions

        VertCount = 4;
        VertByteCount = VertCount * sizeof(glm::vec2);
        std::vector<glm::vec2> QuadVerts({
            glm::vec2(-1,-1),
            glm::vec2( 1,-1),
            glm::vec2( 1, 1),
            glm::vec2(-1, 1),
        });

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, VertByteCount, (const GLvoid*)QuadVerts.data(), GL_STATIC_DRAW);

        // get the buffer addr and then make it resident
        glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &VBOAddr);
        glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);
    }

    // create the shaders for the quad
    {
        std::map<GLuint, std::string> shaders;
        shaders[GL_VERTEX_SHADER] = base_dir + "xor.vert";
        shaders[GL_FRAGMENT_SHADER] = base_dir + "xor.frag";
        ProgramID = createProgram(shaders);
    }

    // framebuffer for the quad to draw too
    {
        Framebuffer.TextureNames.resize(1);
        Framebuffer.Target = GL_TEXTURE_RECTANGLE;
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

        //GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
        //glDrawBuffers(1, draw_buffers);

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
        glUseProgram(ProgramID);
        glBindTexture(Framebuffer.Target, TextureName);
    }

    // draw quad
    {
        glBindVertexArray(VAO);
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, VBOAddr, VertByteCount);
        glDrawArrays(GL_TRIANGLE_FAN, 0, VertCount);
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
        std::cout << ":: From TEST XOR -----------------------\n";
        unsigned int size = Framebuffer.Width * Framebuffer.Height * Framebuffer.ComponentCount;
        unsigned int *data = new unsigned int[size];
        glBindTexture(Framebuffer.Target, Framebuffer.TextureNames[0]);
        glGetTexImage(Framebuffer.Target, 0,
            Framebuffer.Format,
            Framebuffer.Type,
            (GLvoid*)data
            );

        for (size_t i=0; i<size; i+=Framebuffer.ComponentCount)
            std::cout << i << "\t: " << data[i] << "\n";
        delete [] data;
    }
}
