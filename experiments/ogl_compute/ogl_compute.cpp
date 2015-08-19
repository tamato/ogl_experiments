/**
    Calulating navier stokes non-compressable fluids using openGL compute shaders.
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
    int WindowWidth = 512;
    int WindowHeight = 512;

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
            QUAD,
            SplatInk,
            Advect,
            Impulse,
            MAX
        };
    }

    namespace pipeline
    {
        enum type
        {
            QUAD,
            SplatInk,
            Advect,
            Impulse,
            MAX
        };
    }

    namespace texture
    {
        enum type
        {
            SplatInk,
            Velocity0,
            Velocity1,
            Pressure,
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

    namespace textureObject
    {
        GLuint Width = WindowWidth;
        GLuint Height = WindowHeight;
        GLuint ComponentCount = 4;
        GLint  InternalFormat = GL_RGBA16F;
        GLint  Format = GL_RGBA;
        GLint  Type = GL_FLOAT;
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint64 BufferAddr[addr::MAX] = {0};
    GLuint Program[program::MAX] = {0};
    GLuint Pipeline[pipeline::MAX] = {0};
    GLuint Texture[texture::MAX] = {0};

    const GLsizei QuadVertCount = 4;
    const GLsizei QuadSize = QuadVertCount * sizeof(glm::vec2);
    const glm::vec2 QuadVerts[QuadVertCount] = {
        glm::vec2(-1,-1),
        glm::vec2( 1,-1),
        glm::vec2( 1, 1),
        glm::vec2(-1, 1),
    };

    glm::uvec3 LocalWorkGroupSize;
    glm::ivec2 MousePresses[2] = {glm::ivec2(-999), glm::ivec2(-999)};
    
    // shader loc's
    int MouseShaderLoc;
    int DeltaTimeLoc;
    int ImpulsePositionLoc;
    int ForceLoc;

    bool CaptureMouse = false;
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

void mouseCallback(GLFWwindow* window, int btn, int action, int mods)
{
    if (btn == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
        CaptureMouse = true;
        double x,y;
        glfwGetCursorPos(glfwWindow, &x, &y);
        MousePresses[0] = glm::ivec2((int)x,WindowHeight-(int)y);
        MousePresses[1] = MousePresses[0];
    }
    if (btn == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
        CaptureMouse = false;
        MousePresses[0] = glm::ivec2(-999);
        MousePresses[1] = MousePresses[0];
    }
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

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "OpenGL Compute, Fluids", NULL, NULL );
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
    glfwSetMouseButtonCallback(glfwWindow, mouseCallback);
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

        // compute
        ,"GL_ARB_compute_shader"                // https://www.opengl.org/registry/specs/ARB/compute_shader.txt

        // image load store
        ,"GL_ARB_shader_image_load_store"       // https://www.opengl.org/registry/specs/ARB/shader_image_load_store.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required but not supported on this machine." << endl;
    }
}

/**
    This texture will be used to tell the indirect
    rendering commands how many cubes to draw
*/
void initTexture()
{
    unsigned int size = textureObject::Width * textureObject::Height * textureObject::ComponentCount;
    unsigned int *data = new unsigned int[size];
    for (int i=0; i<size; i+=4)
        data[i] = 0;

    glGenTextures(texture::MAX, Texture);
    for (GLuint i=0; i<texture::MAX; ++i){
        glBindTexture(GL_TEXTURE_2D, Texture[i]);

        // Nvidia has a bug that if these were intergal textures
        // GL_LINEAR cannot be used and must be GL_NEAREST
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexImage2D(
            GL_TEXTURE_2D, 0,
            textureObject::InternalFormat,
            textureObject::Width,
            textureObject::Height,
            0,
            textureObject::Format,
            textureObject::Type,
            data
        );

        if (glGetError() != GL_NONE) assert(0);
    }

    delete [] data;
    glBindTexture(GL_TEXTURE_2D, 0);
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
        std::cerr   << "Shader <" << fileName
                    << "> Compile with erros:\n"
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

void computeShaderStats()
{
    cout << "[!] Compute Shader stats" << endl;
    // // GetIntegerv
    // MAX_COMPUTE_UNIFORM_BLOCKS;
    // MAX_COMPUTE_TEXTURE_IMAGE_UNITS;
    // MAX_COMPUTE_IMAGE_UNIFORMS;
    // MAX_COMPUTE_SHARED_MEMORY_SIZE;
    // MAX_COMPUTE_UNIFORM_COMPONENTS;
    // MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS;
    // MAX_COMPUTE_ATOMIC_COUNTERS;
    // MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS;
    // MAX_COMPUTE_WORK_GROUP_INVOCATIONS;

    // // GetIntegeri_v
    // MAX_COMPUTE_WORK_GROUP_COUNT;
    // MAX_COMPUTE_WORK_GROUP_SIZE;

    // // GetProgramiv
    // COMPUTE_WORK_GROUP_SIZE;
    int data[3];
    glGetProgramiv(Program[program::SplatInk], GL_COMPUTE_WORK_GROUP_SIZE, data);
    LocalWorkGroupSize = glm::uvec3(data[0],data[1],data[2]);
    cout << "Size: " << glm::to_string(LocalWorkGroupSize) << endl;

    // // GetActiveUniformBlockiv
    // UNIFORM_BLOCK_REFERENCED_BY_COMPUTE_SHADER;

    // // GetActiveAtomicCounterBufferiv
    // ATOMIC_COUNTER_BUFFER_REFERENCED_BY_COMPUTE_SHADER;
}

void initComputeShader(string&& name, int program_id, int pipeline_id)
{
    GLuint compute = createShader(GL_COMPUTE_SHADER, DataDirectory + name + ".compute");

    Program[program_id] = glCreateProgram();
    glProgramParameteri(Program[program_id], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(Program[program_id], compute);
    glLinkProgram(Program[program_id]);
    glDeleteShader(compute);

    checkShaderLinkage(Program[program_id]);
    glUseProgramStages(Pipeline[pipeline_id], GL_COMPUTE_SHADER_BIT, Program[program_id]);
}

void initQuadShader()
{
    GLuint vert = createShader(GL_VERTEX_SHADER, DataDirectory + "quad.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, DataDirectory + "result.frag");

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

    initTexture();

    createGLObjects();

    initFullScreenQuad();

    initComputeShader("ink", program::SplatInk, pipeline::SplatInk);
    initComputeShader("advect", program::Advect, pipeline::Advect);
    initComputeShader("impulse", program::Impulse, pipeline::Impulse);
    computeShaderStats();

    MouseShaderLoc = glGetUniformLocation(Program[program::SplatInk], "InkSpot");
    DeltaTimeLoc = glGetUniformLocation(Program[program::Advect], "DeltaTime");
    ImpulsePositionLoc = glGetUniformLocation(Program[program::Impulse], "ImpulsePosition");
    ForceLoc = glGetUniformLocation(Program[program::Impulse], "Force");
}

void dispatchSplatInk()
{
    glBindProgramPipeline(Pipeline[pipeline::SplatInk]);

    glProgramUniform2iv(Program[program::SplatInk], MouseShaderLoc, 2, (const GLint*)&MousePresses);

    glBindImageTexture(0, Texture[texture::SplatInk], 0, GL_FALSE, 0, GL_READ_WRITE, textureObject::InternalFormat);
    
    glDispatchCompute(textureObject::Width/LocalWorkGroupSize.x,textureObject::Height/LocalWorkGroupSize.y,LocalWorkGroupSize.z);

    glBindProgramPipeline(0);
}

void dispatchAdvect(size_t quantity, size_t advection, float deltaTime)
{
    glBindProgramPipeline(Pipeline[pipeline::Advect]);

    glProgramUniform1f(Program[program::Advect], DeltaTimeLoc, deltaTime);

    glBindImageTexture(0, Texture[quantity], 0, GL_FALSE, 0, GL_READ_WRITE, textureObject::InternalFormat);
    glBindImageTexture(1, Texture[advection], 0, GL_FALSE, 0, GL_READ_ONLY, textureObject::InternalFormat);
    
    glDispatchCompute(textureObject::Width/LocalWorkGroupSize.x,textureObject::Height/LocalWorkGroupSize.y,LocalWorkGroupSize.z);

    glBindProgramPipeline(0);
}

void dispatchImpulse(int tex)
{
    glBindProgramPipeline(Pipeline[pipeline::Impulse]);

    glm::vec4 impulsePoint = glm::vec4(MousePresses[0], 0, 0);
    glm::vec4 force = glm::vec4(MousePresses[0], 0, 0) - glm::vec4(MousePresses[1], 0, 0);
    force *= 0.1f;
    if (glm::length(force) > 0.1f)
        force = glm::normalize(force);

    glProgramUniform4f(Program[program::Impulse], ImpulsePositionLoc, impulsePoint[0],impulsePoint[1],impulsePoint[2],impulsePoint[3]);
    glProgramUniform4f(Program[program::Impulse], ForceLoc, force[0], force[1], force[2], force[3]);

    glBindImageTexture(0, Texture[tex], 0, GL_FALSE, 0, GL_READ_WRITE, textureObject::InternalFormat);
    
    glDispatchCompute(textureObject::Width/LocalWorkGroupSize.x,textureObject::Height/LocalWorkGroupSize.y,LocalWorkGroupSize.z);

    glBindProgramPipeline(0);
}

void renderquad(int tex)
{
    glClearColor( 0,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT );

    glBindProgramPipeline(Pipeline[pipeline::QUAD]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture[tex]);

    glBindVertexArray(VAO[vao::QUAD]);

    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindProgramPipeline(0);
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        glfwSetTime(0);
        glfwPollEvents();

        if (CaptureMouse)
        {
            double x,y;
            glfwGetCursorPos(glfwWindow, &x, &y);
            MousePresses[1] = MousePresses[0];
            MousePresses[0] = glm::ivec2((int)x,WindowHeight-(int)y);
        }

        float dt = .1f;
        dispatchSplatInk();
        static int t0 = texture::Velocity0;
        static int t1 = texture::Velocity1;
        dispatchAdvect(t0, t1, dt);
        std::swap(t0, t1);
        dispatchAdvect(texture::SplatInk, t1, dt);
        dispatchImpulse(t1);
        // dispatchProject
        renderquad(texture::SplatInk);

        glfwSwapBuffers(glfwWindow);
    }
}

void shutdown()
{
    glDeleteTextures(::texture::MAX, Texture);

    glDeleteProgramPipelines(::pipeline::MAX, Pipeline);
    
    for (int i=0; i<::program::MAX; ++i)
        glDeleteProgram(Program[i]);

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