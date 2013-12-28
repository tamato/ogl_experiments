/**
    run tests to ensure xor operations are done correctly.

    Create an integer framebuffer
    init framebuffer to a specific value
    create a texture with a specific value
    draw a fullscreen quad to the framebuffer with the texture
        using glLogicOp
    test if results are as predicted.
*/
#ifndef TEST_XOR_H_
#define TEST_XOR_H_

#define GLEW_NO_GLU
#include <GL/glew.h>

#include <string>

#include "common.h"

namespace ogle {
    class TestXOR {
        public:
            TestXOR();
            ~TestXOR();

            void init(const std::string& base_dir);
            void run();

        private:
            ogle::Framebuffer Framebuffer;
            GLuint TextureName;

            GLuint ProgramID;
            GLuint VBO;
            GLuint VAO;
            GLuint64 VBOAddr;
            GLsizei VertCount;
            GLsizei VertByteCount;
    };
}

#endif // TEST_XOR_H_