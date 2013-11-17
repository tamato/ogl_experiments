/**
    Create a
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
            MAX
        };
    }

    namespace program
    {
        enum type
        {
            SIGNLE,
            SEPARABLE,
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

    struct Framebuffer
    {
        GLuint  TextureName;
        GLuint  FramebufferName;
        GLuint  ComponentCount;
        GLint   InternalFormat;
        GLsizei Width;
        GLsizei Height;
        GLenum  Format;
        GLenum  Type;
    };

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint64 BufferAddr[addr::MAX] = {0};
    GLuint Program[program::MAX] = {0};

    Framebuffer VoxelData;
    Framebuffer DensityData[2];

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

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "Mipmap Volume Into Density Map", NULL, NULL );
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

        // debug printing support
        ,"GL_ARB_debug_output"                  // http://www.opengl.org/registry/specs/ARB/debug_output.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required but not supported on this machine." << endl;
    }
}

GLuint initTexture(GLint internalFormat, GLuint componentCount, GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    GLuint textureName;
    glGenTextures(1, &textureName);
    glBindTexture(GL_TEXTURE_2D, textureName);

    unsigned int size = width * height * componentCount;
    unsigned int *data = new unsigned int[size];
    for (int i=0; i<size; i+=4)
        data[i] = 0;

    // Nvidia has a bug that if these were intergal textures
    // GL_LINEAR cannot be used and must be GL_NEAREST
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        internalFormat,
        width,
        height,
        0,
        format,
        type,
        data
    );

    if (glGetError() != GL_NONE) assert(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    delete [] data;
    return textureName;
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

void initFramebuffer(Framebuffer& framebuffer)
{
    framebuffer.TextureName = initTexture(
        framebuffer.InternalFormat,
        framebuffer.ComponentCount,
        framebuffer.Width,
        framebuffer.Height,
        framebuffer.Format,
        framebuffer.Type
        );

    glGenFramebuffers(1, &framebuffer.FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.FramebufferName);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer.TextureName, 0);

    // check for completeness
    checkFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initFramebuffers()
{
    initFramebuffer(VoxelData);
    initFramebuffer(DensityData[0]);
    initFramebuffer(DensityData[1]);
}

void createGLObjects()
{
    glGenVertexArrays(::vao::MAX, VAO);
    glGenBuffers(::buffer::MAX, Buffer);
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

    Program[program::SIGNLE] = glCreateProgram();
    glAttachShader(Program[program::SIGNLE], vert);
    glAttachShader(Program[program::SIGNLE], frag);
    glLinkProgram(Program[program::SIGNLE]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkShaderLinkage(Program[program::SIGNLE]);
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

void initFullScreenQuad()
{
    initQuadShader();
    initQuadGeometry();
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

    initFramebuffers();

    createGLObjects();

    initFullScreenQuad();
}

void renderquad()
{
    glClearColor( 0,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // using the following framebuffer, update the atmoic counter
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // and now "draw" the fullscreen quad to the frame buffer to update the atmoic counter
    // for each pixel attempted to be drawn.
    glBindVertexArray(VAO[vao::QUAD]);

    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        glfwSetTime(0);
        renderquad();
        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();
    }
}

void shutdown()
{
    // glDeleteFramebuffers(1, &framebuffer::FramebufferName);
    // glDeleteTextures(1, &framebuffer::TextureName);

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

    /**
        Single pass
            (128*128) * (3 additions)= 49,152 additions

        Multi pass
            (256*128) * (1 addition) = 32,768
                                     +
            (128*128) * (1 addition) = 16,384
                                     = 49,152
    **/
    // make the 1D texture mask
    GLuint componentCount = 4;
    GLuint textureStacks = 1;
    GLuint length = 128 * textureStacks; // depth of voxel map
    GLuint size = sizeof(GLuint) * componentCount * length;
    GLuint *data = new GLuint[size];
    const GLuint R = 0;
    const GLuint G = 1;
    const GLuint B = 2;
    const GLuint A = 3;
    const GLuint depth_mask = 0xFFFFFFFF;
    for (GLuint i=0; i<size; i+=componentCount)
    {
        GLuint red_mask = 0;
        GLuint green_mask = 0;
        GLuint blue_mask = 0;
        GLuint alpha_mask = 0;

        if (i < 32){
            red_mask   = depth_mask >> (i + 1);
            green_mask = depth_mask;
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 64){
            green_mask = depth_mask >> (i - 31);
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 96){
            blue_mask  = depth_mask >> (i - 63);
            alpha_mask = depth_mask;
        }
        else if (i < 128){
           alpha_mask = depth_mask >> (i - 95);
        }

        data[i*componentCount + R] = red_mask;
        data[i*componentCount + G] = green_mask;
        data[i*componentCount + B] = blue_mask;
        data[i*componentCount + A] = alpha_mask;
    }
}