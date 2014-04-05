/**
    Create a app that voxelizes data into the bits of a texture for volume and density
    The volume texture is 4 component 32 bit unsigned ints (128 bits)
    The density texture is actually 2 textures, each the same size as the volume texture.
    The density texture can also be used as a distance field.

    Based on the paper: "Single-Pass GPU Solid Voxelization for Real-Time Applications"
    http://maverick.inria.fr/Publications/2008/ED08a/solidvoxelizationAuthorVersion.pdf

    Things left to do:
    Test out repeated xor'ing (currently its done just once)
    Update tests to actually test data, not just print to console
    Fill out MaskTexture to be used for logic ops
    move "render_depth_mask" to its own file
    display 3D geometry
    write 3D geometry to volume texture
    visualize volume texture -- maybe use CSG for testing?
    density volume
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <bitset>

#include "common.h"
#include "debug.h"
#include "test_xor.h"
#include "objloader.h"

using namespace std;

namespace {
    std::string DataDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;
    int WindowWidth = 800;
    int WindowHeight = 640;
    std::string WindowTitle = "SinglePass Voxelization";

    namespace vao
    {
        enum type
        {
            QUAD,
            MESH,
            MAX
        };
    }

    namespace buffer
    {
        enum type
        {
            QUAD,
            MESH0_POSITIONS,
            MESH0_NORMALS,
            MESH0_INDICES,
            MAX
        };
    }

    namespace program
    {
        enum type
        {
            SINGLE,
            DEPTH_TEST,
            SEPARABLE,
            MESH0,
            MAX
        };
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint Program[program::MAX] = {0};

    ogle::Framebuffer VoxelData;
    ogle::Framebuffer DensityData;
    ogle::Framebuffer DepthMask;

    const GLsizei QuadVertCount = 4;
    const GLsizei QuadSize = QuadVertCount * sizeof(glm::vec2);
    const glm::vec2 QuadVerts[QuadVertCount] = {
        glm::vec2(-1,-1),
        glm::vec2( 1,-1),
        glm::vec2( 1, 1),
        glm::vec2(-1, 1),
    };

    ogle::TestXOR Test_XOR_Ops;

    GLuint VertCount = 0;
    GLuint IndexCount = 0;

    vector<glm::vec3> Positions;
    vector<glm::vec3> Normals;

    struct BoundingBox
    {
        glm::vec3 Center;
        glm::vec3 Extents;
    };
    BoundingBox SceneBoundingBox;
    glm::mat4 SceneTransform;
    struct ProjectionPoD
    {
        float Fov;
        float Near;
        float Far;
    } ProjectionData;
    ogle::ShaderProgram MeshShader;

    /*
    struct Renderable
    {
        BoundingBox;
        Transform;

        struct Geometry {
            vao_name;
            buffer_names;

            // for draw* commands
            vert_count;
            index_count;
        };

        struct MaterialObject{
            uint material_name;     // index into a list of all materials

            struct RenderState{
                uint state_name;    // index into a list of all the different rendering states
            };

            struct ShaderProgram{
                program_name;
                uniform_locations;
            };
        }
    };
    */
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

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, WindowTitle.c_str(), NULL, NULL );
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
        // integer textures
        ,"GL_ARB_geometry_shader4"              // http://www.opengl.org/registry/specs/ARB/geometry_shader4.txt
        ,"GL_EXT_texture_integer"               // http://developer.download.nvidia.com/opengl/specs/GL_EXT_texture_integer.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required but not supported on this machine." << endl;
    }
}

void initFramebuffers()
{
    VoxelData.InternalFormat = GL_RGBA32UI;
    VoxelData.Target = GL_TEXTURE_2D;
    VoxelData.ComponentCount = 4;
    VoxelData.Width = WindowWidth;
    VoxelData.Height = WindowHeight;
    VoxelData.Format = GL_RGBA_INTEGER;
    VoxelData.Type = GL_UNSIGNED_INT;
    VoxelData.TextureNames.resize(1);
    ogle::initFramebuffer(VoxelData);

    DensityData.InternalFormat = GL_RGBA32I;
    DensityData.Target = GL_TEXTURE_2D;
    DensityData.ComponentCount = 4;
    DensityData.Width = WindowWidth;
    DensityData.Height = WindowHeight;
    DensityData.Format = GL_RGBA_INTEGER;
    DensityData.Type = GL_UNSIGNED_INT;
    DensityData.TextureNames.resize(2);
    ogle::initFramebuffer(DensityData);
}

void createGLObjects()
{
    glGenVertexArrays(::vao::MAX, VAO);
    glGenBuffers(::buffer::MAX, Buffer);
}

void initQuadShader()
{
    GLuint vert = ogle::createShader(GL_VERTEX_SHADER, DataDirectory + "quad.vert");
    GLuint frag = ogle::createShader(GL_FRAGMENT_SHADER, DataDirectory + "quad.frag");

    Program[program::SINGLE] = glCreateProgram();
    glAttachShader(Program[program::SINGLE], vert);
    glAttachShader(Program[program::SINGLE], frag);
    glLinkProgram(Program[program::SINGLE]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    ogle::checkShaderLinkage(Program[program::SINGLE]);
}

void initQuadGeometry()
{
    glBindVertexArray(VAO[vao::QUAD]);
    glEnableVertexAttribArray(0); // positions

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::QUAD]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::QUAD]);
    glBufferData(GL_ARRAY_BUFFER, QuadSize, (const GLvoid*)&QuadVerts[0], GL_STATIC_DRAW);
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

void initWriteToIntTexTest()
{
    // creates an int texture
    // creates fbo with int texture
    //  int texture 32 wide, 1 tall,
    // GL_R8UI  GL_RED_INTEGER  ui8
    DepthMask.Target = GL_TEXTURE_2D;
    DepthMask.Width = 32;
    DepthMask.Height = 1;
    DepthMask.ComponentCount = 1;
    DepthMask.InternalFormat = GL_R32UI; // !!!!
    DepthMask.Format = GL_RED_INTEGER;
    DepthMask.Type = GL_UNSIGNED_INT; // !!!!!
    DepthMask.TextureNames.resize(1);
    initFramebuffer(DepthMask);

    // later use
    //  maybe 128 wide?

    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER]   = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "depth_mask_test.frag";
    Program[program::DEPTH_TEST] = ogle::createProgram( shaders );
}

BoundingBox get_bounding_box(const std::vector<glm::vec3>& positions)
{
    glm::vec3 min( 23e9f);
    glm::vec3 max(-23e9f);
    for (size_t i=0; i<positions.size(); ++i){
        min = glm::min(positions[i], min);
        max = glm::max(positions[i], max);
    }

    BoundingBox box;
    box.Center = (max + min) * 0.5f;
    box.Extents = max - box.Center;
    return box;
}

void initMeshShaders()
{
    GLuint vert = ogle::createShader(GL_VERTEX_SHADER, DataDirectory + "mesh.vert");
    GLuint frag = ogle::createShader(GL_FRAGMENT_SHADER, DataDirectory + "mesh.frag");

    Program[program::MESH0] = glCreateProgram();
    glAttachShader(Program[program::MESH0], vert);
    glAttachShader(Program[program::MESH0], frag);
    glLinkProgram(Program[program::MESH0]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    ogle::checkShaderLinkage(Program[program::MESH0]);
    MeshShader.ProgramName = Program[program::MESH0];
    MeshShader.collectUniforms();
}

void initMesh()
{
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "venus.obj");
    VertCount = (GLuint)loader.getVertCount();
    size_t position_attribute_size = loader.getPositionAttributeSize();
    size_t position_bytes = VertCount * position_attribute_size;

    Positions.resize(VertCount);
    const float* positions = loader.getPositions();
    for (size_t i=0; i<VertCount; ++i){
        Positions[i] = glm::vec3(positions[i*3+0], positions[i*3+1], positions[i*3+2]);
    }

    size_t normal_attribute_size = loader.getNormalAttributeSize();
    size_t normal_bytes = VertCount * normal_attribute_size;
    Normals.resize(VertCount);
    const float* normals = loader.getNormals();
    for (size_t i=0; i<VertCount; ++i){
        Normals[i] = glm::vec3(normals[i*3+0], normals[i*3+1], normals[i*3+2]);
    }

    IndexCount = (GLuint)loader.getIndexCount();
    size_t index_attribute_size = loader.getIndexAttributeSize();
    size_t index_bytes = IndexCount * index_attribute_size;

    SceneBoundingBox = get_bounding_box(Positions);

    // get mesh info into the gpu
    glBindVertexArray(VAO[vao::MESH]);

    glEnableVertexAttribArray(0); // positions
    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::MESH0_POSITIONS]);
    glBufferData(GL_ARRAY_BUFFER, position_bytes, (const GLvoid*)loader.getPositions(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(1); // Normals
    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::MESH0_NORMALS]);
    glBufferData(GL_ARRAY_BUFFER, position_bytes, (const GLvoid*)loader.getNormals(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    const unsigned int *elements = loader.getIndices();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::MESH0_INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, (const GLvoid*)elements, GL_STATIC_DRAW);
    glFinish();
    initMeshShaders();
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
    initMesh();
    SceneTransform = glm::mat4(1.0f);
    ProjectionData.Fov = 1.0f;
    ProjectionData.Near = 1000.0f;
    ProjectionData.Far = 10000.0f;

    initWriteToIntTexTest();
    //Test_XOR_Ops.init(DataDirectory);
}

void render_depth_mask_test()
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glBindVertexArray(VAO[vao::QUAD]);
    glBindFramebuffer(GL_FRAMEBUFFER, DepthMask.FramebufferName);

    // for fbos: GL_COLOR_ATTACHMENT0
    //GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    //glDrawBuffers(1, draw_buffers);
    GLuint clear_color[4] = {0,0,0,0};
    glClearBufferuiv(GL_COLOR, 0, clear_color);
    glClear( GL_COLOR_BUFFER_BIT );

    glViewport(0,0,32,1);

    glUseProgram(Program[program::DEPTH_TEST]);

    // glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);

    glViewport( 0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight );
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // now draw the texture
    glClearColor( 1,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT );
    glUseProgram(Program[program::SINGLE]);
    // glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, BufferAddr[addr::QUAD], QuadSize);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);
    glBindVertexArray(0);

    // read back what was written and lets see the results!
    unsigned int size = DepthMask.Width * DepthMask.Height * DepthMask.ComponentCount;
    unsigned int *data = new unsigned int[size];

    glBindTexture(GL_TEXTURE_2D, DepthMask.TextureNames[0]);
    glGetTexImage(GL_TEXTURE_2D, 0,
        DepthMask.Format,
        DepthMask.Type,
        (GLvoid*)data
        );

    for (size_t i=0; i<size; i+=DepthMask.ComponentCount)
        cout << i << "\t: " << data[i] << "\n";
    delete [] data;
}

glm::mat4 center_scene_in_camera()
{
    glm::vec3 center = SceneBoundingBox.Center;
    glm::vec3 up = glm::vec3(0,1,0);
    glm::vec3 eye = center;
    float theta = ProjectionData.Fov * 0.5f;
    eye.z += glm::length(SceneBoundingBox.Extents) / tanf(theta);
    eye += eye * 0.1f;
    return glm::lookAt(eye, center, up);
}

void render_mesh_to_screen()
{
    // render state
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    glm::mat4 proj = glm::perspective(
        ProjectionData.Fov,
        float(WindowWidth)/float(WindowHeight),
        ProjectionData.Near,
        ProjectionData.Far
        );

    glm::mat4 view = center_scene_in_camera();
    glm::mat4 mv = view * SceneTransform;
    glm::mat4 mvp = proj * mv;

    MeshShader.bind();
    glUniformMatrix4fv(MeshShader.Uniforms["WorldViewProjection"], 1, false, (const GLfloat*)&mvp[0][0]);
    glUniformMatrix4fv(MeshShader.Uniforms["WorldView"], 1, false, (const GLfloat*)&mv[0][0]);

    // add a light to the scene
    {
        glm::vec3 light_position = SceneBoundingBox.Extents;
        light_position.z *= -1.0f;
        glUniform3fv(MeshShader.Uniforms["LightPos"], 1, (const GLfloat*)&light_position);
    }

    glBindVertexArray(VAO[vao::MESH]);
    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);
}

void render_fullscreen_quad()
{
    glUseProgram(Program[program::SINGLE]);
    glBindVertexArray(VAO[vao::QUAD]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, QuadVertCount);
}

void render_to_screen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport( 0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight );
    glClearColor( 0.1f,0.1f,0.2f,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    render_mesh_to_screen();
    //render_fullscreen_quad();
}
void render()
{
    /**
     * the following is to test the results of XOR ops
     */
    // render_depth_mask_test();
    // Test_XOR_Ops.run();
    // exit(EXIT_SUCCESS);
    // return;
    // end test xor ops

    // for fbos: GL_COLOR_ATTACHMENT0
    // GLenum draw_buffers[1] = {GL_BACK};
    // glDrawBuffers(1, draw_buffers);
    // GLuint clear_color[4] = {0,0,0,0};
    // glClearBufferuiv(GL_COLOR, 0, clear_color);

    render_to_screen();
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        render();
        glfwPollEvents();
        glfwSwapBuffers(glfwWindow);
    }
}

void shutdown()
{
    // glDeleteFramebuffers(1, &framebuffer::FramebufferName);
    // glDeleteTextures(1, &framebuffer::TextureName);
    for (const auto& p : Program)
        glDeleteProgram(p);

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

    #if 0
    // make the 1D texture mask
    GLuint componentCount = 4;
    GLuint rows = 128;
    GLuint stride = sizeof(GLuint) * componentCount;
    GLuint column_length = stride * 8;
    GLuint size = stride * rows;
    GLuint *data = new GLuint[size];
    const GLuint R = 0;
    const GLuint G = sizeof(GLuint)+R;
    const GLuint B = sizeof(GLuint)+G;
    const GLuint A = sizeof(GLuint)+B;
    const GLuint depth_mask = 0xFFFFFFFF;
    for (GLuint i=0; i<column_length; ++i)
    {
        GLuint red_mask = 0;
        GLuint green_mask = 0;
        GLuint blue_mask = 0;
        GLuint alpha_mask = 0;

        if (i < 32){
            red_mask   = depth_mask >> (i);
            green_mask = depth_mask;
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 64){
            green_mask = depth_mask >> (i - 32);
            blue_mask  = depth_mask;
            alpha_mask = depth_mask;
        }
        else if (i < 96){
            blue_mask  = depth_mask >> (i - 64);
            alpha_mask = depth_mask;
        }
        else if (i < 128){
           alpha_mask = depth_mask >> (i - 96);
        }

        data[i*stride + R] = red_mask;
        data[i*stride + G] = green_mask;
        data[i*stride + B] = blue_mask;
        data[i*stride + A] = alpha_mask;
        cout << bitset<32>(red_mask)
             << bitset<32>(green_mask)
             << bitset<32>(blue_mask)
             << bitset<32>(alpha_mask)
             << endl;
    }
    delete [] data;
    #endif
    exit( EXIT_SUCCESS );
}