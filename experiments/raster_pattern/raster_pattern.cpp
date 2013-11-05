/**
    Experiment to learn how to work with indirect rendering techniques.

    Indirect rendering is when rendering commands are issued from the GPU.

    To practice, a cube will be drawn in the center of the screen
    Then once that is working, the cube will be instanced to draw 4 times
    taking an even amount of space on the screen (1 in each quadrant)
    Then any multiple of 4.

    The number of times the cube will be drawn is controlled by the size of
    the framebuffer the fullscreen quad is being "drawn" with.
    The drawn is in quotes because the quad is not actually drawn at all.
    The framebuffer used with the quad does not write out to the texture attached to it
    However, in the fragment shader atmoic writes are being used to write to a buffer object
    that holds the command arguments that controls how many objects to draw.
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

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
    std::string DataDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;
    int WindowWidth = 800;
    int WindowHeight = 640;

    namespace vao
    {
        enum type
        {
            QUAD,
            MAX
        };
    }

    namespace buffer
    {
        enum type
        {
            QUAD,
            ATOMIC,
            ATOMIC_2,
            MAX
        };
    }

    namespace program
    {
        enum type
        {
            INC_ATOMIC,
            QUAD,
            MAX
        };
    }

    namespace pipeline
    {
        enum type
        {
            INC_ATOMIC,
            QUAD,
            MAX
        };
    }

    namespace addr
    {
        enum type
        {
            QUAD,
            MAX
        };
    }

    namespace buffer_base_loc
    {
        enum type
        {
            ATOMIC,
            ATOMIC_2
        };
    }

    namespace framebuffer
    {
        GLuint TextureName = 0;
        GLuint FramebufferName = 0;
        GLuint Width = WindowWidth;
        GLuint Height = WindowHeight;
        GLuint ComponentCount = 1;
        GLint  InternalFormat = GL_RED;
        GLint  Format = GL_RED;
        GLint  Type = GL_UNSIGNED_INT;
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint64 BufferAddr[addr::MAX] = {0};
    GLuint Program[program::MAX] = {0};
    GLuint Pipeline[pipeline::MAX] = {0};

    const GLsizei QuadVertCount = 4;
    const GLsizei QuadSize = QuadVertCount * sizeof(glm::vec2);
    const glm::vec2 QuadVerts[QuadVertCount] = {
        glm::vec2(-1,-1),
        glm::vec2( 1,-1),
        glm::vec2( 1, 1),
        glm::vec2(-1, 1),
    };
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
    glfwSetErrorCallback(errorCallback);

    /* Init GLFW */
    if( !glfwInit() )
        exit( EXIT_FAILURE );

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "Round Trip Demo", NULL, NULL );
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
        // refer to buffer objects by gpu address
         "GL_NV_shader_buffer_load"             // http://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
        ,"GL_NV_vertex_buffer_unified_memory"   // http://developer.download.nvidia.com/opengl/specs/GL_NV_vertex_buffer_unified_memory.txt

        // mix and match shader stages into a pipeline object
        ,"GL_ARB_separate_shader_objects"       // http://www.opengl.org/registry/specs/ARB/separate_shader_objects.txt

        // debug printing support
        ,"GL_ARB_debug_output"                  // http://www.opengl.org/registry/specs/ARB/debug_output.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required but not supported on this machine." << endl;
    }
}

void createGLObjects()
{
    glGenVertexArrays(::vao::MAX, VAO);
    glGenBuffers(::buffer::MAX, Buffer);
    glGenProgramPipelines(::pipeline::MAX, Pipeline);
}

GLuint createShader(GLuint type, const std::string &fileName)
{
    GLuint shader = glCreateShader(type);

    // load up the file
    std::ifstream inf(fileName);
    if (!inf.is_open()) {
        cerr << "Failed to open " << fileName << endl;
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

void initQuadShader()
{
    GLuint vert = createShader(GL_VERTEX_SHADER, DataDirectory + "quad.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, DataDirectory + "quad.frag");

    Program[program::QUAD] = glCreateProgram();
    glProgramParameteri(Program[program::QUAD], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(Program[program::QUAD], vert);
    glAttachShader(Program[program::QUAD], frag);
    glLinkProgram(Program[program::QUAD]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkShaderLinkage(Program[program::QUAD]);
    glUseProgramStages(Pipeline[pipeline::QUAD], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, Program[program::QUAD]);
}

void initQuadShaderAtomic()
{
    GLuint vert = createShader(GL_VERTEX_SHADER, DataDirectory + "quad.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, DataDirectory + "atomic_inc.frag");

    Program[program::INC_ATOMIC] = glCreateProgram();
    glProgramParameteri(Program[program::INC_ATOMIC], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(Program[program::INC_ATOMIC], vert);
    glAttachShader(Program[program::INC_ATOMIC], frag);
    glLinkProgram(Program[program::INC_ATOMIC]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkShaderLinkage(Program[program::INC_ATOMIC]);
    glUseProgramStages(Pipeline[pipeline::INC_ATOMIC], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, Program[program::INC_ATOMIC]);
}

void initQuadGeometry()
{
    glBindVertexArray(VAO[vao::QUAD]);
    glVertexAttribFormatNV(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2));

    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glEnableVertexAttribArray(0); // positions

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::QUAD]);
    glBufferData(GL_ARRAY_BUFFER, QuadSize, (const GLvoid*)&QuadVerts[0], GL_STATIC_DRAW);

    // get the buffer addr and then make it resident
    glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &BufferAddr[addr::QUAD]);
    glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);
}

void initAtomicBuffers()
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::ATOMIC]);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::ATOMIC_2]);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void initAtomicUniform()
{
    // use the name of the block(struct) to get its index
    GLint counter_loc = glGetUniformBlockIndex(Program[program::QUAD], "atomic");
    // this should only be called once, tell the program what binding location to match with the block index
    glUniformBlockBinding(Program[program::QUAD], counter_loc, buffer_base_loc::ATOMIC);    // sets state in the glsl program

    GLint blockSize = 0;
    glGetActiveUniformBlockiv(
        Program[program::QUAD],
        counter_loc,
        GL_UNIFORM_BLOCK_DATA_SIZE,
        &blockSize
    );

    glBindBuffer(GL_UNIFORM_BUFFER, Buffer[buffer::ATOMIC]);
    glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, buffer_base_loc::ATOMIC, Buffer[buffer::ATOMIC]);
}

void initFullScreenQuad()
{
    initQuadShader();
    initQuadShaderAtomic();
    initQuadGeometry();
    initAtomicBuffers();
    initAtomicUniform();
}

void setDataDir(int argc, char *argv[])
{
    // get base directory for reading in files
    std::string path = argv[0];
    std::replace(path.begin(), path.end(), '\\', '/');
    size_t dir_idx = path.rfind("/")+1;
    std::string exe_dir = path.substr(0, dir_idx);
    std::string exe_name = path.substr(dir_idx);
    DataDirectory = exe_dir + "../data/" + exe_name + "/";
}

void init( int argc, char *argv[])
{
    setDataDir(argc, argv);

    initGLFW();
    initGLEW();
    checkExtensions();
    ogle::Debug::init();

    createGLObjects();

    initFullScreenQuad();
}

void increment_counter()
{
    // Clear out the atmoic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::ATOMIC]);
    unsigned int data = 0;
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), (GLvoid*)&data);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // the main goal of this render pass is to write to the atomic counter, turn off what we are not going to use.
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // and now "draw" the fullscreen quad to the frame buffer to update the atmoic counter
    // for each pixel attempted to be drawn.
    glBindProgramPipeline(Pipeline[pipeline::INC_ATOMIC]);
    glBindVertexArray(VAO[vao::QUAD]);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, buffer_base_loc::ATOMIC, Buffer[buffer::ATOMIC]);

    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);

    glBindVertexArray(0);
    glBindProgramPipeline(0);
}

void renderquad()
{
    // Clear out the atmoic counter
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::ATOMIC_2]);
    unsigned int data = 0;
    glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(unsigned int), (GLvoid*)&data);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor( 0,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // and now "draw" the fullscreen quad to the frame buffer to update the atmoic counter
    // for each pixel attempted to be drawn.
    glBindProgramPipeline(Pipeline[pipeline::QUAD]);
    glBindVertexArray(VAO[vao::QUAD]);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, buffer_base_loc::ATOMIC_2, Buffer[buffer::ATOMIC_2]);

    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);

    glBindVertexArray(0);
    glBindProgramPipeline(0);
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        increment_counter();
        renderquad();
        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();
    }
}

void shutdown()
{
    glDeleteFramebuffers(1, &framebuffer::FramebufferName);
    glDeleteTextures(1, &framebuffer::TextureName);

    glDeleteProgramPipelines(::pipeline::MAX, Pipeline);
    glDeleteBuffers(::buffer::MAX, Buffer);
    glDeleteVertexArrays(::vao::MAX, VAO);

    ogle::Debug::shutdown();

    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

int main( int argc, char *argv[])
{
    init(argc, argv);
    runloop();
    shutdown();
    exit( EXIT_SUCCESS );
}