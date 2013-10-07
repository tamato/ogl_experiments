/***************************************************
    Demo app to see the speed difference between
    using bound vertex arrays and bindless
****************************************************/    

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <map>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <gputimer.h>

using namespace std;

/** throw things into the unnamed namespace */
namespace {
    std::string BaseDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;

    const int QuadVerts = 6;
    const int VertexCount = QuadVerts * 10000;
    GLuint VBOCount = 10;
    GLuint BoundVao;
    vector<GLuint> VBO_Bound(VBOCount);
    GLuint Bound_Program = 0;

    GLuint BindlessVao;
    vector<GLuint> VBO_Bindless(VBOCount);
    vector<GLuint64> VBO_Addrs(VBOCount);
    GLuint Bindless_Program = 0;

    struct vec3 {
        vec3():x(0),y(0),z(0){}
        vec3(float _x, float _y, float _z):x(_x),y(_y),z(_z){}
        float x,y,z;
    };

    map<int, string> OGLDebugSource;
    map<int, string> OGLDebugType;
    map<int, string> OGLDebugSeverity;
    map<int, string> OGLDebugIDs;

    vec3 DataInit[VertexCount];    
}

void error_callback(int error, const char* description)
{
    cerr << description << endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

/** makes sure that we can use all the extensions we need for this app */
void checkExtensions(){
    /** 
        the extesnsions that are going to be need are for bindless vbo's which are:
    */
    vector<string> extensions {
         "GL_NV_shader_buffer_load"             // http://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
        ,"GL_NV_vertex_buffer_unified_memory"   // http://developer.download.nvidia.com/opengl/specs/GL_NV_vertex_buffer_unified_memory.txt
        ,"GL_ARB_separate_shader_objects"       // http://www.opengl.org/registry/specs/ARB/separate_shader_objects.txt
        ,"GL_ARB_timer_query"                   // http://www.opengl.org/registry/specs/ARB/timer_query.txt
        ,"GL_ARB_debug_output"                  // http://www.opengl.org/registry/specs/ARB/debug_output.txt
    };
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE)
            cerr << extension << " - is required and not supported on this machine." << endl;
    }
}

void debugOutput(
    unsigned int source, 
    unsigned int type, 
    unsigned int id, 
    unsigned int severity, 
    int length, 
    const char* message, 
    void* userParam){

    cout << "OGL Debugger Error: \n"
         << "\tSource: "    << OGLDebugSource[source] << "\n"
         << "\tType: "      << OGLDebugType[type] << "\n"
         << "\tID: "        << OGLDebugIDs[id] << "\n"
         << "\tSeverity: "  << OGLDebugSeverity[severity] << "\n"
         << "\tMessage:"    << message 
         << endl;
}

/** Sets up opengl debugging capabilities */
void initDebug(){
    #define GL_ENUM_TO_STRING(enum) #enum
    OGLDebugSource[GL_DEBUG_SOURCE_API_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_API_ARB);
    OGLDebugSource[GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB);
    OGLDebugSource[GL_DEBUG_SOURCE_SHADER_COMPILER_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_SHADER_COMPILER_ARB);
    OGLDebugSource[GL_DEBUG_SOURCE_THIRD_PARTY_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_THIRD_PARTY_ARB);
    OGLDebugSource[GL_DEBUG_SOURCE_APPLICATION_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_APPLICATION_ARB);
    OGLDebugSource[GL_DEBUG_SOURCE_OTHER_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SOURCE_OTHER_ARB);

    OGLDebugType[GL_DEBUG_TYPE_ERROR_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_ERROR_ARB);
    OGLDebugType[GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB);
    OGLDebugType[GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB);
    OGLDebugType[GL_DEBUG_TYPE_PORTABILITY_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_PORTABILITY_ARB);
    OGLDebugType[GL_DEBUG_TYPE_PERFORMANCE_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_PERFORMANCE_ARB);
    OGLDebugType[GL_DEBUG_TYPE_OTHER_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_TYPE_OTHER_ARB);

    OGLDebugSeverity[GL_DEBUG_SEVERITY_HIGH_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SEVERITY_HIGH_ARB);
    OGLDebugSeverity[GL_DEBUG_SEVERITY_MEDIUM_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SEVERITY_MEDIUM_ARB);
    OGLDebugSeverity[GL_DEBUG_SEVERITY_LOW_ARB] = GL_ENUM_TO_STRING(GL_DEBUG_SEVERITY_LOW_ARB);

    OGLDebugIDs[GL_INVALID_ENUM] = GL_ENUM_TO_STRING(GL_INVALID_ENUM);
    OGLDebugIDs[GL_INVALID_VALUE] = GL_ENUM_TO_STRING(GL_INVALID_VALUE);
    OGLDebugIDs[GL_INVALID_OPERATION] = GL_ENUM_TO_STRING(GL_INVALID_OPERATION);
    OGLDebugIDs[GL_STACK_OVERFLOW] = GL_ENUM_TO_STRING(GL_STACK_OVERFLOW);
    OGLDebugIDs[GL_STACK_UNDERFLOW] = GL_ENUM_TO_STRING(GL_STACK_UNDERFLOW);
    OGLDebugIDs[GL_OUT_OF_MEMORY] = GL_ENUM_TO_STRING(GL_OUT_OF_MEMORY);
    #undef GL_ENUM_TO_STRING

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB(debugOutput, nullptr);
}

/** creates our Window */
void initGLFW(){
    int width, height;

    /* Init GLFW */
    if( !glfwInit() )
        exit( EXIT_FAILURE );

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( 400, 400, "Bindless NV Example", NULL, NULL );
    if (!glfwWindow)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSwapInterval( 1 );

    glfwGetFramebufferSize(glfwWindow, &width, &height);
    glViewport( 0, 0, (GLsizei)width, (GLsizei)height );

    glfwSetTime( 0.0 );
    glfwSetKeyCallback(glfwWindow, key_callback);
    glfwSetErrorCallback(error_callback);
}

/** inits glew for us */
void initGLEW(){
    glewExperimental=GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        /// if it fails becuase there is no current opengl version, 
        /// don't call glewInit() until after a context has been made current.
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit( EXIT_FAILURE );
    }
    glGetError();    
}

/** init some plain jane VBO's */
void initOldVBOs() {
    // get some safe default data
    vec3 bl(-1.0f, -1.0f, 0.0f);
    vec3 br( 1.0f, -1.0f, 0.0f);
    vec3 tr( 1.0f,  1.0f, 0.0f);
    vec3 tl(-1.0f,  1.0f, 0.0f);
    for (int i=0; i<VertexCount; i+=QuadVerts){
        DataInit[i+0] = bl;
        DataInit[i+1] = br;
        DataInit[i+2] = tr;
        DataInit[i+3] = tr;
        DataInit[i+4] = tl;
        DataInit[i+5] = bl;
    }

    glGenVertexArrays(1, &BoundVao);
    glBindVertexArray(BoundVao);
    glEnableVertexAttribArray(0); // positions

    // nothing fancy, just position data
    glGenBuffers(VBOCount, VBO_Bound.data());
    for (GLuint i=0; i<VBOCount; ++i){
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Bound[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DataInit), (const GLvoid*)&DataInit[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // just position data
    }
}

GLuint createShader(GLuint type, const std::string &fileName){
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

/** shaders that use old vbo's */
void initShadersOldVBOs(){

    GLuint vert = createShader(GL_VERTEX_SHADER, BaseDirectory + "bound.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, BaseDirectory + "bound.frag");

    Bound_Program = glCreateProgram();
    glAttachShader(Bound_Program, vert);
    glDeleteShader(vert);
    glAttachShader(Bound_Program, frag);
    glDeleteShader(frag);
    glLinkProgram(Bound_Program);

    int status;
    glGetProgramiv(Bound_Program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        const int maxLen = 1000;
        int len;
        char errorBuffer[maxLen]{0};
        glGetProgramInfoLog(Bound_Program, maxLen, &len, errorBuffer);
        std::cerr   << "Shader Linked with erros:\n"
                    << errorBuffer << std::endl;
        assert(0);
    }

    glUseProgram(Bound_Program);
}

/** init Bindless VBO's */
void initBindlessVBOs() {
    glGenVertexArrays(1, &BindlessVao);
    glBindVertexArray(BindlessVao);
    glVertexAttribFormatNV(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3));
    
    glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);
    glEnableVertexAttribArray(0); // positions

    // nothing fancy, just position data
    glGenBuffers(VBOCount, VBO_Bindless.data());
    for (GLuint i=0; i<VBOCount; ++i){
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Bindless[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(DataInit), (const GLvoid*)&DataInit[0], GL_STATIC_DRAW);

        // get the buffer addr and then make it resident
        glGetBufferParameterui64vNV(GL_ARRAY_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &VBO_Addrs[i]);
        glMakeBufferResidentNV(GL_ARRAY_BUFFER, GL_READ_ONLY);
    }
}

/** init shaders that use bindless VBOs */
void initShadersBindless(){
    GLuint vert = createShader(GL_VERTEX_SHADER, BaseDirectory + "bindless.vert");
    GLuint frag = createShader(GL_FRAGMENT_SHADER, BaseDirectory + "bindless.frag");

    Bindless_Program = glCreateProgram();
    glAttachShader(Bindless_Program, vert);
    glDeleteShader(vert);
    glAttachShader(Bindless_Program, frag);
    glDeleteShader(frag);
    glLinkProgram(Bindless_Program);

    int status;
    glGetProgramiv(Bindless_Program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        const int maxLen = 1000;
        int len;
        char errorBuffer[maxLen]{0};
        glGetProgramInfoLog(Bindless_Program, maxLen, &len, errorBuffer);
        std::cerr   << "Shader Linked with erros:\n"
                    << errorBuffer << std::endl;
        assert(0);
    }

    glUseProgram(Bindless_Program);
}

/** calls other init functions and may do some other init'ing as well */
void init( int argc, char *argv[] ){
    initGLFW();
    initGLEW();
    checkExtensions();

    initDebug();    

    // get base directory for reading in files
    BaseDirectory = std::string(argv[0]);
    BaseDirectory += "_data/";

    // now get to the real inits.
    // init some VBO's the normal way
    initOldVBOs();

    // normal shaders
    initShadersOldVBOs();

    // bindless vbos
    initBindlessVBOs();

    // shaders that use bindless vbo's
    initShadersBindless();
}

/** tells the gpu to run its commands again
 * since we are not drawing anything to the screen it would be odd to call this method "draw()" or "display()" 
 */
void run_cycle_bound(){
    glBindVertexArray(BoundVao);
    glUseProgram(Bound_Program);

    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClearColor( 0,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    for (GLuint i=0; i<VBOCount; ++i){
        glBindBuffer(GL_ARRAY_BUFFER, VBO_Bound[i]);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // just position data
        glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    }
}

void run_cycle_bindless(){
    glBindVertexArray(BindlessVao);
    glUseProgram(Bindless_Program);

    GLsizeiptr vertex_bytes = sizeof(DataInit);

    glClearColor( 0,0,0,0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    for (GLuint i=0; i<VBOCount; ++i){
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, 0, VBO_Addrs[i], vertex_bytes);
        glDrawArrays(GL_TRIANGLES, 0, VertexCount);
    }
}

void shutdown(){
}

typedef void (APIENTRY *run_cycles)();
void run_test( const string& test_name, run_cycles callback ){
    int iteration_count = 10;
    int loop_counter = 0;
    double start = glfwGetTime();
    double gpu_total_time = 0.0f;

    sp::gpuTimer gpuTimer;
    gpuTimer.init();
    
    cout << "Starting " << test_name << " iterations" << endl;    
    
    /* Main loop */
    while (loop_counter++ < iteration_count)
    {
        gpuTimer.start();
        callback();
        gpuTimer.end();
        gpu_total_time += gpuTimer.elaspedTime();

        /* Swap buffers */
        glfwSwapBuffers(glfwWindow);
        glfwPollEvents();

        /* Check if we are still running */
        if (glfwWindowShouldClose(glfwWindow))
            break;

        cout << ".";
        cout.flush();
    }
    cout << "\n"
         << "Finished " << test_name << " iterations\n"
         << "\ttime to complete " << glfwGetTime() - start << "\n"
         << "\tgpu time " << gpu_total_time << endl;

    cout << "Clearing out gpu queries..." << endl;
    double additional_time = 0.0;
    while (gpuTimer.activeQueries() > 0){
        additional_time += gpuTimer.elaspedTime();
    }
    cout << "Additional Time: " << additional_time << endl;    
}

int main( int argc, char *argv[]) {
    init(argc, argv);


    run_test("Bound VBO's", run_cycle_bound);
    run_test("Bindless VBO's", run_cycle_bindless);

    shutdown();
    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwTerminate();
    exit( EXIT_SUCCESS );
}