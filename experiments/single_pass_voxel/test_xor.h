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

#include <string>

#include "common.h"

namespace ogle {
class TestXOR {
    public:
        TestXOR();
        ~TestXOR();

        void init(const std::string& base_dir);
        void run();
        void shutdown();

    private:
        ogle::Framebuffer Framebuffer;
        GLuint TextureName;

        FullscreenQuad Quad;
        ShaderProgram Program;
};
}

#endif // TEST_XOR_H_
