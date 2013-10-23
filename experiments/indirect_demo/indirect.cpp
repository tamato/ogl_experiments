/**
    Experiment to learn how to work with indirect rendering techniques.

    Indirect rendering is when rendering commands are issued from the GPU.

    To practice, a cube will be drawn in the center of the screen
    Then once that is working, the cube will be instanced to draw 4 time
    taking an even amount of space on the screen (1 in each quadrant)
    Then any multiple of 4.

    The number of times the cube will be drawn is controlled by the size of
    the framebuffer the fullscreen quad is being "drawn" with.
    The drawn is in quotes because the quad is not actually drawn at all.
    The framebuffer used with the quad has does not write out to the texture attached to it
    However, in the fragment shader atmoic writes are being used to write to a buffer object
    that holds the command arguments that controls how many objects to draw.
*/
#include <iostream>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX11
// #define GLM_MESSAGES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debug.h"

using namespace std;

namespace {
    std::string BaseDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;
    int WindowWidth = 800;
    int WindowHeight = 640;

    // namespace for indirect command related properties
    namespace ic { // ic == indirect_commands
        GLuint TextureID = 0;
        GLuint FramebufferID = 0;
        GLsizei PrimCount = 0;  // # of objects to draw
        GLsizei TextureSize = 1;
        GLsizei Count = TextureSize*TextureSize;      // # of verts to draw
    }

    namespace vao
    {
        enum type
        {
            CUBE,
            QUAD,
            MAX
        };
    }

    namespace buffer
    {
        enum type
        {
            CUBE,
            QUAD,
            INDIRECT,
            MAX
        };
    }

    namespace program
    {
        enum type
        {
            CUBE,
            QUAD,
            MAX
        };
    }

    GLuint VAO[vao::MAX];
    GLuint Buffer[buffer::MAX];
    GLuint Program[program::MAX];

    const GLsizei QuadVertCount = 4;
    const GLsizei QuadSize = QuadVertCount * sizeof(glm::vec2);
    const glm::vec2 QuadVerts[QuadVertCount] = {
        glm::vec2(-1,-1),
        glm::vec2( 1,-1),
        glm::vec2( 1, 1),
        glm::vec2(-1, 1)
    };

    GLsizei CubeVertCount = 0;
    GLsizei CubeSize = 0;
    glm::vec3 CubeVerts;
}

void errorCallback(int error, const char* description)
{
    cerr << description << endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

void initGLFW()
{
    /* Init GLFW */
    if( !glfwInit() )
        exit( EXIT_FAILURE );

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "Bindless NV Example", NULL, NULL );
    if (!glfwWindow)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSwapInterval( 1 );

    glViewport( 0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight );

    glfwSetTime( 0.0 );
    glfwSetKeyCallback(glfwWindow, keyCallback);
    glfwSetErrorCallback(errorCallback);
}

void initGLEW()
{
    glewExperimental=GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        /// if it fails here, its becuase there is no current opengl version,
        /// don't call glewInit() until after a context has been made current.
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit( EXIT_FAILURE );
    }
    glGetError(); // GLEW has problems, clear that one that it creates.
}

/** makes sure that we can use all the extensions we need for this app */
void checkExtensions()
{
    vector<string> extensions {
        // refer to opengl objects by gpu address
         "GL_NV_shader_buffer_load"             // http://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
        ,"GL_NV_vertex_buffer_unified_memory"   // http://developer.download.nvidia.com/opengl/specs/GL_NV_vertex_buffer_unified_memory.txt

        // mix and match shader stages into a pipeline object
        ,"GL_ARB_separate_shader_objects"       // http://www.opengl.org/registry/specs/ARB/separate_shader_objects.txt

        // debug printing support
        ,"GL_ARB_debug_output"                  // http://www.opengl.org/registry/specs/ARB/debug_output.txt

        // indirect rendering (issue rendering commands from the gpu)
        ,"GL_ARB_draw_indirect"                 // http://www.opengl.org/registry/specs/ARB/draw_indirect.txt
        ,"GL_ARB_base_instance"                 // http://www.opengl.org/registry/specs/ARB/base_instance.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required but not supported on this machine." << endl;
    }
}

void initCube()
{

}

void initShader()
{
    /** using a shader pipeline object, yay modern openGL objects! */
}

/**
    This texture will be used to tell the indirect
    rendering commands how many cubes to draw
*/
void initTexture()
{
    glGenTextures(1, &::ic::TextureID);
    glBindTexture(GL_TEXTURE_2D, ::ic::TextureID);

    int num_componets = 4;
    unsigned int size = ::ic::TextureSize * ::ic::TextureSize * num_componets;
    unsigned int *data = new unsigned int[size];
    for (int i=0; i<size; i+=4)
        data[i] = 0;

    // Nvidia has a bug that if these were intergal textures
    // GL_LINEAR cannot be used and must be GL_NEAREST
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ::ic::TextureSize, ::ic::TextureSize, 0, GL_RGBA, GL_UNSIGNED_INT, data);

    if (glGetError() != GL_NONE) assert(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    delete [] data;
}

void checkFBO()
{
    GLenum result = glCheckFramebufferStatus( GL_FRAMEBUFFER );
    if (result != GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "FBO incomplete, error: " << endl;
        switch (result)
        {
            /*********************************************************************
              These values were found from:
                http://www.opengl.org/wiki/GLAPI/glCheckFramebufferStatus
            *********************************************************************/
            case GL_FRAMEBUFFER_UNDEFINED:
                cout << "\tGL_FRAMEBUFFER_UNDEFINED\n";
                cout << "\t-target- is the default framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n";
                cout << "\tthe framebuffer attachment is incomplete";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n";
                cout << "\tthere are no images attached to the framebuffer";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n";
                cout << "\tone of the color attaches has an object type of GL_NONE";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n";
                cout << "\tGL_READ_BUFFER attached object type of GL_NONE";
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                cout << "\tGL_FRAMEBUFFER_UNSUPPORTED\n";
                cout << "\tinternal formats conflict with implementation-dependent restrictions";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n";
                cout << "\tis also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS"
                     << "\tis not the same for all attached textures; or, if the attached images"
                     << "\tare a mix of renderbuffers and textures, the value of"
                     << "\tGL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.";
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                cout << "\tGL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n";
                cout << "\ta framebuffer attachment is layered, and a populated attachment is not layered,"
                     << "\tor if all populated color attachments are not from textures of the same target.";
                break;
            default:
                cout << "\tUnknown error occured.";
        }
        cout << endl;
        assert(0);
    }
}

void initFramebuffer()
{
    glGenFramebuffers(1, &::ic::FramebufferID);
    glBindFramebuffer(GL_FRAMEBUFFER, ::ic::FramebufferID);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ::ic::TextureID, 0);

    // check for completeness
    checkFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bindFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, ::ic::FramebufferID);
    if ( ::ic::FramebufferID != 0 )
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ::ic::TextureID, 0);

    // make sure nothing broke.
    checkFBO();
}

void initFullScreenQuad()
{

}

void initQuadShader()
{

}

void initGLSettings()
{
}

void init( int argc, char *argv[])
{
    initGLFW();
    initGLEW();
    checkExtensions();
    ogle::Debug::init();

    initTexture();
    initFramebuffer();
    initFullScreenQuad();
}

void shutdown()
{
    glDeleteFramebuffers(1, &::ic::FramebufferID);
    glDeleteTextures(1, &::ic::TextureID);

    ogle::Debug::shutdown();
    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwTerminate();
}

int main( int argc, char *argv[])
{
    init(argc, argv);
    shutdown();
    exit( EXIT_SUCCESS );
}