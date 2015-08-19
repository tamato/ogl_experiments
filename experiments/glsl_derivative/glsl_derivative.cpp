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
    string WindowName = "ShowDerivates";

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
            DERIVATIVES,
            CROSS_DERIVATES,
            CROSS_COTANGENTS,
            MAX
        };
    }

    GLuint VAO[vao::MAX] = {0};
    GLuint Buffer[buffer::MAX] = {0};
    GLuint Program[program::MAX] = {0};
    GLuint VertCount = 0;
    GLuint IndexCount = 0;

    vector<glm::vec3> Positions;
    GLint TransformsLoc = 0;

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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, WindowName.c_str(), NULL, NULL );
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

void render()
{
    float far = 1000.0f;
    float near = 0.1f;
    glm::mat4 Projection = glm::perspective(45.0f, 4.0f / 3.0f, near, far);
    glm::mat4 View = center_scene(SceneBoundingBox, 45.0f);
    glm::mat4 Model = glm::mat4(1.0f);

    glm::vec4 eye_pos = View[3];
    glm::vec4 lookat = glm::vec4(SceneBoundingBox.Center, 1);
    glm::vec4 dir = lookat - eye_pos;

    float scalar = sin( (float)glfwGetTime() ) * 0.3 + 1.6f;
    eye_pos = dir * scalar;
    View = glm::lookAt(glm::vec3(-eye_pos), SceneBoundingBox.Center, {0,1,0});

    glm::mat4 MVP = Projection * glm::inverse(View) * Model;

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::POSITIONS]);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::INDICES]);

    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3,0.5,0.7,0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glUseProgram(Program[program::DERIVATIVES]);
    glUniformMatrix4fv(TransformsLoc, 1, false, (const GLfloat*)&MVP[0][0]);

    glDrawRangeElements(GL_TRIANGLES, 0, VertCount, IndexCount, GL_UNSIGNED_INT, 0);
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

void initDerivativeShader()
{
    initShader(program::DERIVATIVES, "derivatives");
    TransformsLoc = glGetUniformLocation(Program[program::DERIVATIVES], "Transforms");
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
    loader.load(DataDirectory + "../geometry/sphere.obj");
    VertCount = (GLuint)loader.getVertCount();
    size_t position_attribute_size = loader.getPositionAttributeSize();
    size_t position_bytes = VertCount * position_attribute_size;

    Positions.resize(VertCount);
    const float* positions = loader.getPositions();
    for (size_t i=0; i<VertCount; ++i){
        Positions[i] = glm::vec3(positions[i*3+0], positions[i*3+1], positions[i*3+2]);
    }

    IndexCount = (GLuint)loader.getIndexCount();
    size_t index_attribute_size = loader.getIndexAttributeSize();
    size_t index_bytes = IndexCount * index_attribute_size;

    SceneBoundingBox = get_bounding_box(Positions);

    glBindVertexArray(VAO[vao::MESH]);
    glEnableVertexAttribArray(0); // positions

    glBindBuffer(GL_ARRAY_BUFFER, Buffer[buffer::POSITIONS]);
    glBufferData(GL_ARRAY_BUFFER, position_bytes, (const GLvoid*)loader.getPositions(), GL_STATIC_DRAW);

    const unsigned int *elements = loader.getIndices();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Buffer[buffer::INDICES]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_bytes, (const GLvoid*)elements, GL_STATIC_DRAW);
    glFinish();
}

void init(int argc, char* argv[])
{
    setDataDir(argc, argv);
    initGLFW();
    initGLEW();
    ogle::Debug::init();
    initGLObjects();

    initDerivativeShader();
    initMesh();
}

int main(int argc, char* argv[])
{
    init(argc, argv);
    runloop();
    shutdown();
    exit( EXIT_SUCCESS );
}