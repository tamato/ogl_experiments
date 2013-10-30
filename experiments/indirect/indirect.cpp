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
#include <string>
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

#include "cubegenerator.h"
#include "debug.h"

using namespace std;

namespace {
    std::string DataDirectory; // ends with a forward slash
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
            CUBE_NORMALS,
            CUBE_INDICES,
            CUBE_TRANSFORM,
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

    namespace pipeline
    {
        enum type
        {
            CUBE,
            QUAD,
            MAX
        };
    }

    namespace addr
    {
        enum type
        {
            CUBE_POSITIONS,
            CUBE_NORMALS,
            CUBE_INDICES,
            QUAD,
            MAX
        };
    }

    namespace uniformblock
    {
        enum type
        {
            TRANSFORM
        };
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

    GLsizei CubeVertCount = 0;
    GLsizei CubeVertSize = 0;
    GLsizei CubeIndiceSize = 0;
    GLsizei CubeIndiceCount = 0;
    GLuint  CubeTransformBlockIdx = 0;
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

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "Indirect Demo", NULL, NULL );
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

        // indirect rendering (issue rendering commands from the gpu)
        ,"GL_ARB_draw_indirect"                 // http://www.opengl.org/registry/specs/ARB/draw_indirect.txt
        ,"GL_ARB_base_instance"                 // http://www.opengl.org/registry/specs/ARB/base_instance.txt

        // for atomic operations on the gpu
        ,"GL_ARB_shader_atomic_counters"        // http://www.opengl.org/registry/specs/ARB/shader_atomic_counters.txt

        // clear buffer objects (why this hasn't always existed is surprising)
        ,"GL_ARB_clear_buffer_object"           // http://www.opengl.org/registry/specs/ARB/clear_buffer_object.txt
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

void initAtomicBuffer()
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::INDIRECT]);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void initFullScreenQuad()
{
    initQuadShader();
    initQuadGeometry();
    initAtomicBuffer();
}

void initCubeShader()
{
    GLuint vert = createShader(GL_VERTEX_SHADER, DataDirectory + "cube.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, DataDirectory + "cube.frag");

    Program[program::CUBE] = glCreateProgram();
    glProgramParameteri(Program[program::CUBE], GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(Program[program::CUBE], vert);
    glAttachShader(Program[program::CUBE], frag);
    glLinkProgram(Program[program::CUBE]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkShaderLinkage(Program[program::CUBE]);
    glUseProgramStages(Pipeline[pipeline::CUBE], GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, Program[program::CUBE]);
}

void initCubeGeometry()
{
    glBindVertexArray(VAO[vao::CUBE]);
    glVertexAttribFormatNV(0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribFormatNV(1, 3, GL_FLOAT, GL_FALSE, 0);

    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    glEnableVertexAttribArray(0); // positions
    glEnableVertexAttribArray(1); // normals

    ogle::CubeGenerator geom;
    // geom.tessellation_density(16);
    geom.generate();

    CubeVertCount = geom.Positions.size();
    CubeVertSize = sizeof(glm::vec3) * CubeVertCount;

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::CUBE]);
    glBufferData(GL_ARRAY_BUFFER, CubeVertSize, (const GLvoid*)&geom.Positions[0], GL_STATIC_DRAW);
    glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &BufferAddr[addr::CUBE_POSITIONS]);
    glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::CUBE_NORMALS]);
    glBufferData(GL_ARRAY_BUFFER, CubeVertSize, (const GLvoid*)&geom.Normals[0], GL_STATIC_DRAW);
    glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &BufferAddr[addr::CUBE_NORMALS]);
    glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);

    CubeIndiceCount = geom.Indices.size();
    CubeIndiceSize = sizeof(unsigned int) * CubeIndiceCount;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::CUBE_INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, CubeIndiceSize, (const GLvoid*)&geom.Indices[0], GL_STATIC_DRAW);
    glGetBufferParameterui64vNV(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &BufferAddr[addr::CUBE_INDICES]);
    glMakeBufferResidentNV(GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY);

    glFinish(); // ensure that the geometry makes it to the server before the client side memory goes away.

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
}

void initCubeUniforms()
{
    // use the name of the block(struct) to get its index
    CubeTransformBlockIdx = glGetUniformBlockIndex(Program[program::CUBE], "transform");
    // this should only be called once, tell the program what binding location to match with the block index
    glUniformBlockBinding(Program[program::CUBE], CubeTransformBlockIdx, uniformblock::TRANSFORM);    // sets state in the glsl program

    GLint blockSize = 0;
    glGetActiveUniformBlockiv(
        Program[program::CUBE],
        CubeTransformBlockIdx,
        GL_UNIFORM_BLOCK_DATA_SIZE,
        &blockSize
    );

    glBindBuffer(GL_UNIFORM_BUFFER, Buffer[buffer::CUBE_TRANSFORM]);
    glBufferData(GL_UNIFORM_BUFFER, blockSize, 0, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void initCube()
{
    initCubeShader();
    initCubeGeometry();
    initCubeUniforms();
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

void init( int argc, char *argv[] )
{
    setDataDir(argc, argv);
    initGLFW();
    initGLEW();
    checkExtensions();
    ogle::Debug::init();

    initTexture();
    initFramebuffer();

    createGLObjects();

    initFullScreenQuad();
    initCube();
}

void renderquad()
{

    // Clear out the atmoic counter
    GLuint clear_data = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::INDIRECT]);
    glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_data);

    // the main goal of this render pass is to write to the atomic counter, turn off what we are not going to use.
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // using the following framebuffer, update the atomic counter
    bindFBO();

    // and now "draw" the fullscreen quad to the frame buffer to update the atmoic counter
    // for each pixel attempted to be drawn.
    glBindProgramPipeline(Pipeline[pipeline::QUAD]);
    glBindVertexArray(VAO[vao::QUAD]);
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, Buffer[buffer::INDIRECT]);
    glVertexAttribFormatNV(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2)); // <- need to be here?

    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);
    glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);

    glBindVertexArray(0);
    glBindProgramPipeline(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void rendercube()
{
    {
    GLuint clear_data = 0;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::INDIRECT]);
    glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear_data);
    }

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor( 0,1,0,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glDisable(GL_CULL_FACE);

    {
    glDisable(GL_DEPTH_TEST);
    }

    glm::mat4 Projection = glm::mat4(1.0f);
    // glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
    glm::mat4 View = glm::mat4(1.0f);
    glm::mat4 Model = glm::mat4(1.0f);

    static float y=0;
    y += 0.003f;
    Model = glm::rotate(Model, y, glm::vec3(0.f, 1.f, 0.f));
    Model[3][3] = 15.0f;
    glm::mat4 MVP = Projection * View * Model;

    // todo: figure out if bindbufferbase is needed at all if the binding point is already set and we are using glbindbuffer
    glBindBufferBase(GL_UNIFORM_BUFFER, uniformblock::TRANSFORM, Buffer[buffer::CUBE_TRANSFORM]);
    glBindBuffer(GL_UNIFORM_BUFFER, Buffer[buffer::CUBE_TRANSFORM]);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &MVP[0][0]);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), &Model[0][0]);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindProgramPipeline(Pipeline[pipeline::CUBE]);
    glBindVertexArray(VAO[vao::CUBE]);
    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);

    {
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, Buffer[buffer::INDIRECT]);
    }

    // these only need to be done if the vertex format changes, which it does between drawing the quad and this
    // Does this need to be done, being that VAO's are used?
    {
    glVertexAttribFormatNV(0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribFormatNV(1, 3, GL_FLOAT, GL_FALSE, 0);
    }

    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::CUBE_POSITIONS], CubeVertCount);
    glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 1, BufferAddr[addr::CUBE_NORMALS], CubeVertCount);

    glEnableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::CUBE_INDICES], CubeIndiceCount);
    glDrawElements(GL_TRIANGLES, CubeIndiceCount, GL_UNSIGNED_INT, nullptr);

    glDisableClientState(GL_ELEMENT_ARRAY_UNIFIED_NV);
    glDisableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glBindVertexArray(0);
    glBindProgramPipeline(0);

    {
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
        GLuint *counter;
        glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::INDIRECT]);
        counter = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        cout << "Counter : " << counter[0] << endl;
        glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    }
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        renderquad();
        rendercube();

        // test if buffer was written to
        // GLuint *counter;
        // glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, Buffer[buffer::INDIRECT]);
        // counter = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT);
        // cout << "Counter : " << counter[0] << endl;
        // glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();
    }
}

void shutdown()
{
    glDeleteFramebuffers(1, &::ic::FramebufferID);
    glDeleteTextures(1, &::ic::TextureID);

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