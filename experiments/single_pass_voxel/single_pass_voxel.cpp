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
#include "test_integer_texture.h"
#include "objloader.h"

using namespace std;

namespace {
    std::string DataDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;
    int WindowWidth = 512;
    int WindowHeight = 512;
    std::string WindowTitle = "SinglePass Voxelization";

    namespace vao
    {
        enum type
        {
            MESH,
            MAX
        };
    }

    namespace buffer
    {
        enum type
        {
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
            MESH0,
            MAX
        };
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint Program[program::MAX] = {0};

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
    glm::mat4 Projection;
    glm::mat4 View;
    glm::mat4 MV;
    glm::mat4 MVP;
    glm::mat4 SceneTransform;
    struct ProjectionPoD
    {
        float Fov;
        float Near;
        float Far;
    } ProjectionData;

    ogle::FullscreenQuad Quad;
    ogle::ShaderProgram FS_Shader;
    ogle::ShaderProgram Density_FS_Shader;

    ogle::ShaderProgram MeshShader;

    ogle::Framebuffer VoxelData;
    ogle::ShaderProgram VoxelShader;
    ogle::Framebuffer DensityData;
    ogle::ShaderProgram DensityShader;
    ogle::ShaderProgram DensityNormalShader;
    GLuint BitMask;
    GLuint DensityBitMask;

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
        // debug printing support
        "GL_ARB_debug_output"                   // http://www.opengl.org/registry/specs/ARB/debug_output.txt
        // integer textures
        ,"GL_ARB_geometry_shader4"              // http://www.opengl.org/registry/specs/ARB/geometry_shader4.txt
                                                // for usampler2D and isampler2D
        ,"GL_EXT_texture_integer"               // http://developer.download.nvidia.com/opengl/specs/GL_EXT_texture_integer.txt
                                                // for RGBA32UI_EXT
        // default binding values in shaders for textures
        ,"GL_ARB_shading_language_420pack"      // http://www.opengl.org/registry/specs/ARB/shading_language_420pack.txt
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
}

void initFullScreenQuad()
{
    Quad.init();
    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "quad.frag";
    FS_Shader.init(shaders);

    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "blit_density.frag";
    Density_FS_Shader.init(shaders);
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
    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "mesh.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "mesh.frag";
    MeshShader.init(shaders);
}

void initMesh()
{
    // load up mesh
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "bunny.obj");
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

void initVoxelShader()
{
    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "voxel.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "voxel.frag";
    VoxelShader.init(shaders);
}

void initBitMaskTexture()
{
    glGenTextures(1, &BitMask);
    glBindTexture(GL_TEXTURE_2D, BitMask);

    // make the 1D texture mask
    GLuint stride = 4;
    GLuint width = 128;
    GLuint size = stride * width;
    GLuint *data = new GLuint[size];
    const GLuint R = 0;
    const GLuint G = 1;
    const GLuint B = 2;
    const GLuint A = 3;
    const GLuint depth_mask = -1;
    GLuint red_mask = 0;
    GLuint green_mask = 0;
    GLuint blue_mask = 0;
    GLuint alpha_mask = 0;

    cout << "sizeof(gluint) " << sizeof(GLuint) << std::endl;

    for (GLuint i=0; i<width; ++i)
    {
        if (i<32){
            red_mask   = depth_mask << (31 - i);
        }
        else if (i<64){
            green_mask = depth_mask << (63 - i);
        }
        else if (i<96){
            blue_mask  = depth_mask << (95 - i);
        }
        else if (i<128){
           alpha_mask = depth_mask << (127 - i);
        }
        data[i*stride + R] = red_mask;
        data[i*stride + G] = green_mask;
        data[i*stride + B] = blue_mask;
        data[i*stride + A] = alpha_mask;
        // cout << "I: " << i << "\t"
        //      << "R:" << bitset<32>(red_mask) << " "
        //      << "G:" << bitset<32>(green_mask) << " "
        //      << "B:" << bitset<32>(blue_mask) << " "
        //      << "A:" << bitset<32>(alpha_mask)
        //      << endl;
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA32UI,
        width,
        1,
        0,
        GL_RGBA_INTEGER,
        GL_UNSIGNED_INT,
        data
    );

    if (glGetError() != GL_NONE) assert(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    delete [] data;
}

void initDensityShader()
{
    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "density.frag";
    DensityShader.init(shaders);
}

void initDensityNormalShader()
{
    std::map<GLuint, std::string> shaders;
    shaders[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    shaders[GL_FRAGMENT_SHADER] = DataDirectory + "density_normal.frag";
    DensityNormalShader.init(shaders);
}

void initDensityBitMaskTexture()
{
    glGenTextures(1, &DensityBitMask);
    glBindTexture(GL_TEXTURE_2D, DensityBitMask);

    // make the 1D texture mask
    GLuint stride = 4;
    GLuint width = 32;
    GLuint size = stride * width;
    GLuint *data = new GLuint[size];
    const GLuint R = 0;
    const GLuint G = 1;
    const GLuint B = 2;
    const GLuint A = 3;
    const GLuint depth_mask = -1;
    GLuint red_mask = 0;
    GLuint green_mask = 0;
    GLuint blue_mask = 0;
    GLuint alpha_mask = 0;

    cout << "Size of density bit mask: " << size << endl;

    for (GLuint i=0; i<width; ++i)
    {
        auto bit_shift = 32 - (i+1)*4;
        if (i<8){
            red_mask   = depth_mask << bit_shift;
        }
        else if (i<16){
            green_mask = depth_mask << bit_shift;
        }
        else if (i<24){
            blue_mask  = depth_mask << bit_shift;
        }
        else if (i<32){
            alpha_mask = depth_mask << bit_shift;
        }

        data[i*stride + R] = red_mask;
        data[i*stride + G] = green_mask;
        data[i*stride + B] = blue_mask;
        data[i*stride + A] = alpha_mask;
        cout << "I: " << i << "\t"
             << "R:" << bitset<32>(red_mask) << " "
             << "G:" << bitset<32>(green_mask) << " "
             << "B:" << bitset<32>(blue_mask) << " "
             << "A:" << bitset<32>(alpha_mask) << " "
             << endl;
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA32UI,
        width,
        1,
        0,
        GL_RGBA_INTEGER,
        GL_UNSIGNED_INT,
        data
    );

    if (glGetError() != GL_NONE) assert(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    delete [] data;
}

void initVoxel()
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

    DensityData.InternalFormat = GL_RGBA32UI;
    DensityData.Target = GL_TEXTURE_2D;
    DensityData.ComponentCount = 4;
    DensityData.Width = WindowWidth/2;
    DensityData.Height = WindowHeight/2;
    DensityData.Format = GL_RGBA_INTEGER;
    DensityData.Type = GL_UNSIGNED_INT;
    DensityData.TextureNames.resize(VoxelData.TextureNames.size() * 2);
    ogle::initFramebuffer(DensityData);

    initVoxelShader();
    initDensityShader();
    initDensityNormalShader();
    initBitMaskTexture();
    initDensityBitMaskTexture();
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
    initMesh();
    initVoxel();
    SceneTransform = glm::mat4(1.0f);
    ProjectionData.Fov = 1.0f;
}

glm::mat4 center_scene_in_camera()
{
    glm::vec3 center = SceneBoundingBox.Center;
    glm::vec3 up = glm::vec3(0,1,0);
    glm::vec3 eye = center;
    float theta = ProjectionData.Fov * 0.5f;
    eye.z += glm::length(SceneBoundingBox.Extents) / tanf(theta);
    eye *= 1.1f;
    return glm::lookAt(eye, center, up);
}

void render_mesh_to_screen()
{
    // render state
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    MeshShader.bind();
    glUniformMatrix4fv(MeshShader.Uniforms["WorldViewProjection"], 1, false, glm::value_ptr(MVP));
    glUniformMatrix4fv(MeshShader.Uniforms["WorldView"], 1, false, glm::value_ptr(MV));

    // add a light to the scene
    {
        glm::vec3 light_position = SceneBoundingBox.Extents;
        glUniform3fv(MeshShader.Uniforms["LightPos"], 1, glm::value_ptr(light_position));
    }

    glBindVertexArray(VAO[vao::MESH]);
    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);
}

void render_fs_voxel()
{
    glEnable(GL_BLEND);

    FS_Shader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, VoxelData.TextureNames[0]);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, VoxelData.TextureNames[1]);

    Quad.render();
}

void render_fs_density()
{
    glEnable(GL_BLEND);

    Density_FS_Shader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, DensityData.TextureNames[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, DensityData.TextureNames[1]);

    Quad.render();
}

void render_to_screen()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport( 0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight );
    glClearColor( 0.1f,0.1f,0.2f,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // render_mesh_to_screen();
    // render_fs_voxel();
    render_fs_density();
}

void render_mesh_to_voxel()
{
    VoxelShader.bind();
    glUniformMatrix4fv(VoxelShader.Uniforms["WorldViewProjection"], 1, false, glm::value_ptr(MVP));
    glUniform2f(VoxelShader.Uniforms["DepthExtents"], ProjectionData.Near, ProjectionData.Far);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, BitMask);

    glBindVertexArray(VAO[vao::MESH]);
    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);
}

void render_to_voxel()
{
    glBindFramebuffer(GL_FRAMEBUFFER, VoxelData.FramebufferName);
    glViewport( 0, 0, VoxelData.Width, VoxelData.Height );

    size_t buffer_count = VoxelData.TextureNames.size();
    GLuint color[4] = {0,0,0,0};
    for (size_t i=0; i<buffer_count; ++i)
        glClearBufferuiv(GL_COLOR, i, color);

    // render state
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_CLAMP);
        glEnable(GL_COLOR_LOGIC_OP);    // disables all color blending
        glLogicOp(GL_XOR);
    }

    render_mesh_to_voxel();

    // disable render state
    {
        glEnable(GL_CULL_FACE);
        glDisable(GL_COLOR_LOGIC_OP);
        glDisable(GL_DEPTH_CLAMP);
    }
}

void render_to_density()
{
    glBindFramebuffer(GL_FRAMEBUFFER, DensityData.FramebufferName);
    glViewport( 0, 0, DensityData.Width, DensityData.Height );

    size_t buffer_count = DensityData.TextureNames.size();
    GLuint color[4] = {0,0,0,0};
    for (size_t i=0; i<buffer_count; ++i)
        glClearBufferuiv(GL_COLOR, i, color);

    DensityShader.bind();
    for (size_t i=0; i<VoxelData.TextureNames.size(); ++i){
        glActiveTexture(GL_TEXTURE0+i);
        glBindTexture(GL_TEXTURE_2D, VoxelData.TextureNames[i]);
    }

    Quad.render();

    #if 0
    // for tests, read back both density textures and make sure there bits are in the right place
    {
        // the result
        unsigned int size = DensityData.Width * DensityData.Height * DensityData.ComponentCount;
        unsigned int *data = new unsigned int[size];
        cout << "Density texture checks: " <<endl;
        for (size_t i=0; i<DensityData.TextureNames.size(); ++i){
            glBindTexture(DensityData.Target, DensityData.TextureNames[i]);
            glGetTexImage(DensityData.Target, 0,
                DensityData.Format,
                DensityData.Type,
                (GLvoid*)data
                );

            // go through all the entries in data (each component of a texel is an entry)
            for (size_t j=0; j<size; ++j){
                // go through all the nibbles of each entry
                for (size_t k=0; k<8; ++k){

                    // get the nibble
                    unsigned char test = (data[j] >> 4*k) & (0xF);

                    // mask 0111
                    static const GLuint check_mask0 = 0x7;
                    // mask 1000
                    static const GLuint check_mask1 = 0x8;

                    bool b = false;
                    bool a = false;
                    bool too_many_bits_set = false;

                    // check first bit
                    a = (test & check_mask1);
                    b = (test & check_mask0);
                    if (a && b){
                        too_many_bits_set = true;
                    }

                    if (too_many_bits_set){
                        std::cout   << "Failed at index: " << i << " " << j << "\n"
                                    << "Bits: " << bitset<32>(test)
                                    << std::endl;

                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        delete [] data;
    }
    #endif
}

void render()
{
    // double before = glfwGetTime();
    // double after = glfwGetTime();
    // double diff = after - before;
    // cout << "CPU Voxelization time: " << diff*1000.0 << " ms" << endl;

    render_to_voxel();
    render_to_density();
    render_to_screen();
}

void update(double time_passed)
{
    glm::mat4 y_rot = glm::rotate(SceneTransform, /*0.0f*/float(time_passed), glm::vec3(0,1,0));
    View = center_scene_in_camera();

    ProjectionData.Far = -View[3][2]*3.f;
    ProjectionData.Near = ProjectionData.Far * .1f;
    Projection = glm::perspective(
        ProjectionData.Fov,
        float(WindowWidth)/float(WindowHeight),
        ProjectionData.Near,
        ProjectionData.Far
        );

    MV = View * y_rot;
    MVP =  Projection * MV;
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        update(glfwGetTime());
        render();
        glfwPollEvents();
        glfwSwapBuffers(glfwWindow);
    }
}

void shutdown()
{
    for (const auto& p : Program)
        glDeleteProgram(p);

    Quad.shutdown();
    FS_Shader.shutdown();
    Density_FS_Shader.shutdown();
    MeshShader.shutdown();
    VoxelShader.shutdown();
    DensityShader.shutdown();
    DensityNormalShader.shutdown();


    VoxelData.shutdown();
    DensityData.shutdown();
    glDeleteTextures(1, &BitMask);

    glDeleteBuffers(::buffer::MAX, Buffer);
    glDeleteVertexArrays(::vao::MAX, VAO);

    ogle::Debug::shutdown();

    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

void run_tests()
{
    ogle::Test_Integer_Texture TestInt;
    TestInt.init(DataDirectory);
    TestInt.check_int_textur_bound();
    TestInt.read_back_texture();
    TestInt.write_read_to_fbo();
    TestInt.shutdown();

    ogle::TestXOR Test_XOR_Ops;
    Test_XOR_Ops.init(DataDirectory);
    Test_XOR_Ops.run();
    Test_XOR_Ops.shutdown();
}

int main( int argc, char *argv[])
{
    init(argc, argv);
    // run_tests();
    runloop();
    shutdown();
    exit( EXIT_SUCCESS );
}
