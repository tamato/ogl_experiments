#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "debug.h"
#include "objloader.h"

using namespace std;

namespace {
    std::string DataDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;
    int WindowWidth = 800;
    int WindowHeight = 640;

    GLuint TextureName = 0;
    GLuint FramebufferName = 0;

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
            POSITIONS,
            INDICES,
            MAX
        };
    }

    namespace program
    {
        enum type
        {
            MESH,
            DEPTH,
            MAX
        };
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint Program[program::MAX] = {0};
    GLuint VertCount = 0;
    GLuint IndexCount = 0;

    vector<glm::vec3> Positions;
    GLint MVPLoc_Mesh = 0;
    GLint MVPLoc_Depth = 0;

    struct BoundingBox
    {
        glm::vec3 Center;
        glm::vec3 Extents;
    };
    BoundingBox SceneBoundingBox;
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
    // glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, "Scene Depth", NULL, NULL );
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

void initTexture()
{
    glGenTextures(1, &TextureName);
    glBindTexture(GL_TEXTURE_2D, TextureName);

    int num_componets = 4;
    unsigned int size = num_componets;
    float *data = new float[size];
    for (int i=0; i<size; i+=num_componets)
        data[i] = 0;

    // Nvidia has a bug that if these were intergal textures
    // GL_LINEAR cannot be used and must be GL_NEAREST
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, data);

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
    glGenFramebuffers(1, &FramebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureName, 0);

    // check for completeness
    checkFBO();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void bindFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
    if ( FramebufferName != 0 )
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TextureName, 0);

    // make sure nothing broke.
    checkFBO();
}
void initGLObjects()
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

void initShader(int program, const std::string& shader)
{
    GLuint vert = createShader(GL_VERTEX_SHADER, DataDirectory + shader + ".vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, DataDirectory + shader + ".frag");

    Program[program] = glCreateProgram();
    glAttachShader(Program[program], vert);
    glAttachShader(Program[program], frag);
    glLinkProgram(Program[program]);
    glDeleteShader(vert);
    glDeleteShader(frag);

    checkShaderLinkage(Program[program]);
}

void initDepthShader()
{
    initShader(program::DEPTH, "depth");
    MVPLoc_Depth = glGetUniformLocation(Program[program::DEPTH], "MVP");
}

void initMeshShader()
{
    initShader(program::MESH, "mesh");
    MVPLoc_Mesh = glGetUniformLocation(Program[program::MESH], "MVP");
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

void initMesh()
{
    ogle::ObjLoader loader;
    loader.load(DataDirectory + "Anatomy_A.obj");
    // loader.load(DataDirectory + "sphere.obj");
    VertCount = (GLuint)loader.getVertCount();
    size_t position_attribute_size = loader.getPositionAttributeSize();
    size_t position_bytes = VertCount * position_attribute_size;

    IndexCount = (GLuint)loader.getIndexCount();
    size_t index_attribute_size = loader.getIndexAttributeSize();
    size_t index_bytes = IndexCount * index_attribute_size;

    Positions.resize(VertCount);
    const float* positions = loader.getPositions();
    for (size_t i=0; i<VertCount; i+=3){
        Positions[i] = glm::vec3(positions[i*3+0], positions[i*3+1], positions[i*3+1]);
    }

    SceneBoundingBox = get_bounding_box(Positions);

    glBindVertexArray(VAO[vao::MESH]);
    glEnableVertexAttribArray(0); // positions

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::POSITIONS]);
    glBufferData(GL_ARRAY_BUFFER, position_bytes, (const GLvoid*)loader.getPositions(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, (const GLvoid*)loader.getIndices(), GL_STATIC_DRAW);
    glFinish();
}

void init(int argc, char* argv[])
{
    setDataDir(argc, argv);
    initGLFW();
    initGLEW();
    ogle::Debug::init();
    initGLObjects();

    initTexture();
    initFramebuffer();

    initDepthShader();
    initMeshShader();
    initMesh();
}

glm::mat4 center_scene(const BoundingBox& scene, float view_angle_degree)
{
    float rads = glm::radians(view_angle_degree);
    float theta = atan(rads);
    float opposite = glm::length(scene.Extents);
    float adjacent = opposite / theta;
    glm::vec3 eye_pos = scene.Center + glm::vec3(0,0, adjacent);
    glm::mat4 view = glm::lookAt(-eye_pos, scene.Center, {0,1,0});
    return view;
}

void render_depth(const glm::mat4& mvp)
{
    glViewport( 0, 0, 1, 1);
    bindFBO();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // the initial value of blend func is ONE, so just leave it be.
    // update blend equation to get MIN MAX values.
    glBlendEquationSeparate(GL_MIN, GL_MAX);

    glClearColor(1,1,1,0);
    glClear( GL_COLOR_BUFFER_BIT );

    glUseProgram(Program[program::DEPTH]);
    glUniformMatrix4fv(MVPLoc_Depth, 1, false, (const GLfloat*)&mvp[0][0]);

    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void render_mesh(const glm::mat4& mvp)
{
    glViewport( 0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight );
    glClearColor(0.3,0.5,0.7,0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram(Program[program::MESH]);
    glUniformMatrix4fv(MVPLoc_Mesh, 1, false, (const GLfloat*)&mvp[0][0]);

    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);
}

void render_min_max(const glm::mat4& mvp, GLenum blend_eq, const std::string& test)
{
    glViewport( 0, 0, 1, 1);
    bindFBO();
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    // the initial value of blend func is ONE, so just leave it be.
    // update blend equation to get MIN MAX values.
    glBlendEquation(blend_eq);

    float clear_color = (blend_eq == GL_MIN) ? 1 : 0;

    glClearColor(clear_color, clear_color, clear_color, clear_color);
    glClear( GL_COLOR_BUFFER_BIT );

    glUseProgram(Program[program::DEPTH]);
    glUniformMatrix4fv(MVPLoc_Depth, 1, false, (const GLfloat*)&mvp[0][0]);

    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, TextureName);
    float data[4];
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    cout << test << "\t: " << data[0] << endl;
}

void cpu_depth_usage(const glm::mat4& mvp, float far)
{
    // find the nearest and furthest verts
    float nearest=23e8f, furthest=-23e8f;
    for (glm::vec3 pos : Positions){
        glm::vec4 dpos = mvp * glm::vec4(pos, 1);
        // float z = dpos.z/dpos.w;

        const float Fcoef = 2.0 / log2(far + 1.0);
        float w = dpos.w;
        float z = log2(max(1e-6, 1.0 + w)) * Fcoef - 1.0;
        z /= w;
        // z = 0.5f * z + 0.5f;

        nearest  = glm::min(nearest,  z);
        furthest = glm::max(furthest, z);
    }
    cout << "CPU: " << nearest << " " << furthest << endl;
}

void gpu_depth_usage()
{
    static float largest_diff = -100000.0f;
    // get ahold of the values written to the color buffer that has the depth values
    glBindTexture(GL_TEXTURE_2D, TextureName);
    float data[4];
    glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (data[0] < 0.99f && data[3] > 0.01f){
        if (largest_diff < (data[3] - data[0])){
            largest_diff = (data[3] - data[0]);
        }
    }

    cout << "GPU: " << data[0] << " " << data[3]
         << " diff: " << largest_diff
         << endl;
}

void render()
{
    float far = 500.0f;
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, 1.f, far);
    glm::mat4 View = center_scene(SceneBoundingBox, 45.0f);
    glm::mat4 Model = glm::mat4(1.0f);

    glm::vec4 eye_pos = View[3];
    glm::vec4 lookat = glm::vec4(SceneBoundingBox.Center, 1);
    glm::vec4 dir = lookat - eye_pos;

    float scalar = sin( (float)glfwGetTime() ) * 0.1f;// + 0.6f;
    eye_pos = dir * scalar;
    View = glm::lookAt(glm::vec3(-eye_pos), SceneBoundingBox.Center, {0,1,0});

    glm::mat4 MVP = Projection * glm::inverse(View) * Model;

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::POSITIONS]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::INDICES]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    render_mesh(MVP);
    render_depth(MVP);

    cpu_depth_usage(MVP, far);
    gpu_depth_usage();

    render_min_max(MVP, GL_MIN, "near");
    render_min_max(MVP, GL_MAX, "far");
}

void runloop()
{
    while (!glfwWindowShouldClose(glfwWindow)){
        render();
        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();
    }
}

void shutdown()
{
    glDeleteBuffers(::buffer::MAX, Buffer);
    glDeleteVertexArrays(::vao::MAX, VAO);

    for (size_t i=0; i<program::MAX; ++i){
        glDeleteProgram(Program[i]);
    }

    ogle::Debug::shutdown();
    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

int main(int argc, char* argv[])
{
    init(argc, argv);
    runloop();
    shutdown();
    exit( EXIT_SUCCESS );
}