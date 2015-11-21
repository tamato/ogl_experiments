
#include <algorithm>
#include <iostream>
#include <iomanip>      // std::setprecision
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>

#include "common.h"
#include "debug.h"

#include "drawablebufferdata.h"
#include "bufferdata_pre_orphan.h"
#include "buffersubdata_no_orphan.h"
#include "buffersubdata_pre_orphan.h"
#include "buffersubdata_post_orphan.h"
#include "tripplevbo_no_orphan.h"
#include "map_persistent.h"
#include "meshdata.h"
#include "programobject.h"

namespace {
    int WindowWidth = 640;
    int WindowHeight = 480;
    std::string WindowName = "";
    std::string DataDirectory; // ends with a forward slash
    GLFWwindow *glfwWindow;

    enum buffer_update_method
    {
        // pre arb_buffer_storage
        BUFFER_DATA = 0,
        BUFFER_DATA_PRE_ORPHAN,

        BUFFER_SUB_DATA_NO_ORPHAN,
        BUFFER_SUB_DATA_PRE_ORPHAN,
        BUFFER_SUB_DATA_POST_ORPHAN,

        // arb_buffer_storage
        MAP_PERSISTENT,

        // not arb_buffer_storage, just an experiment
        TRIPPLE_VBO_NO_ORPHAN,
        
        MAX
    };

    // ParticleCount creates # of bytes:
    // Verts( 6 * ParticleCount * (3*sizeof(float)))
    // Verts( ParticleCount * 72 )
    unsigned int Count = 400;
    unsigned int ParticleCount = Count*Count;    // bytes: 11520000, ~11 MB
    MeshData Particles;
    ogle::ProgramObject ParticleShader;

    std::vector<std::shared_ptr<DrawableBufferData>> BufferMethod;
    int MethodSelection = 0;
}


void errorCallback(int error, const char* description)
{
    std::cerr << description << std::endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (action == GLFW_RELEASE){
        switch(key){
            case GLFW_KEY_UP:
                MethodSelection = (MethodSelection+1) % buffer_update_method::MAX;
                std::cout << "New selection: " << MethodSelection << std::endl;
            break;
            case GLFW_KEY_DOWN:
            {
                MethodSelection = (MethodSelection - 1 + buffer_update_method::MAX) % buffer_update_method::MAX;
                std::cout << "New selection: " << MethodSelection << std::endl;
                break;
            }
            default:
            break;
        }
    }
}

/** makes sure that we can use all the extensions we need for this app */
bool checkExtensions()
{
    /**
        the extesnsions that are going to be need are for bindless vbo's which are:
    */
    std::vector<std::string> extensions {
        // Allows the use of the gpu to have direct accesss to client memory
        //  bypass gpu memory managent, 
        //  have persistent mappings to buffer objects
        "GL_ARB_buffer_storage"                 // https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
        ,"GL_ARB_map_buffer_range"              // https://www.opengl.org/registry/specs/ARB/map_buffer_range.txt
        ,"GL_ARB_sync"                          // https://www.opengl.org/registry/specs/ARB/sync.txt

        // queries the gpu for time stamps
        ,"GL_ARB_timer_query"                   // http://www.opengl.org/registry/specs/ARB/timer_query.txt

        // debug printing support
        ,"GL_ARB_debug_output"                  // http://www.opengl.org/registry/specs/ARB/debug_output.txt
    };

    bool found_lacking = false;
    for (auto extension : extensions) {
        if (glfwExtensionSupported(extension.c_str()) == GL_FALSE){
            std::cerr << extension << " - is required and not supported on this machine." << std::endl;
            found_lacking = true;
        }
    }

    return !found_lacking;
}

void initGLSettings()
{
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_CULL_FACE);
}

/** creates our Window */
void initGLFW()
{
    /* Init GLFW */
    if( !glfwInit() )
        exit( EXIT_FAILURE );

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindow = glfwCreateWindow( WindowWidth, WindowHeight, WindowName.c_str(), NULL, NULL );
    if (!glfwWindow)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwMakeContextCurrent(glfwWindow);
    glfwSwapInterval( 0 ); // Turn off vsync for benchmarking.
    std::cout << "[!] Warning, be sure that vsync is disabled in NVidia controller panel." << std::endl;
    std::cout << "[!] Having vsync disabled allows for a little faster frame rate, but allows for screen tearing and possibly disables triple buffering of the driver." << std::endl;

    int width, height;
    glfwGetFramebufferSize(glfwWindow, &width, &height);
    glViewport( 0, 0, (GLsizei)width, (GLsizei)height );

    glfwSetTime( 0.0 );
    glfwSetKeyCallback(glfwWindow, keyCallback);
    glfwSetErrorCallback(errorCallback);
}

void setDataDir(int argc, char *argv[]){
    // get base directory for reading in files
    std::string path = argv[0];
    std::replace(path.begin(), path.end(), '\\', '/');
    size_t dir_idx = path.rfind("/")+1;
    std::string exe_dir = path.substr(0, dir_idx);
    std::string exe_name = path.substr(dir_idx);
    DataDirectory = exe_dir + "../data/" + exe_name + "/";
    WindowName = exe_name;
}

void fillParticleMeshData(){

    const int points_per_quad = 6;
    // Particles.VertData.resize(ParticleCount * points_per_quad);
    Particles.VertByteCount = ParticleCount * points_per_quad * sizeof(glm::vec3);
    Particles.VertData = new char[Particles.VertByteCount];
    memset((void*)Particles.VertData, 0, Particles.VertByteCount);
    
    int rows = Count;
    int colums = Count;

    float width = 1 / (float)colums;
    float height = 1 / (float)rows;
    float offset_width = width * 2.f;
    float offset_height = height * 2.f;

    for (int r=0; r<rows; ++r){
        for (int c=0; c<colums; ++c){
            
            float ptx = (c * width) + (width * 0.0f);
            ptx = (ptx - .5f) * 2.f;
            float pty = (r * height) + (height * 0.0f);
            pty = (pty - .5f) * 2.f;

            glm::vec3 bl = glm::vec3(ptx,pty,0);
            glm::vec3 tl = glm::vec3(ptx,pty+offset_height,0);
            glm::vec3 tr = glm::vec3(ptx+offset_width,pty+offset_height,0);
            glm::vec3 br = glm::vec3(ptx+offset_width,pty,0);
            glm::vec3 points[6] = {bl,br,tr, bl,tr,tl};

            int index = (r * colums * sizeof(points)) + (c * sizeof(points));
            memcpy((void*)&Particles.VertData[index], (void*)points, sizeof(points));
        }
    }
}

void initParticleShaders(){
    std::map<unsigned int, std::string> source;
    source[GL_VERTEX_SHADER] = DataDirectory + "quad.vert";
    source[GL_FRAGMENT_SHADER] = DataDirectory + "quad.frag";

    ParticleShader.init(source);
}

void createParticles(){
    fillParticleMeshData();
    initParticleShaders();
}

void createBufferObject(){
    BufferMethod.push_back(std::make_shared<MapPersistent>());
    BufferMethod.push_back(std::make_shared<DrawableBufferData>());
    BufferMethod.push_back(std::make_shared<BufferdataPreOrphan>());
    BufferMethod.push_back(std::make_shared<BufferSubDataNoOrphan>());
    BufferMethod.push_back(std::make_shared<BufferSubDataPreOrphan>());
    BufferMethod.push_back(std::make_shared<BufferSubDataPostOrphan>());
    // BufferMethod.push_back(std::make_shared<TrippleVBONoOrphan>());

    for (auto& buff : BufferMethod){
        buff->init(Particles);
    }
}

bool init(int argc, char *argv[]){

    setDataDir(argc, argv);
    initGLFW();
    ogle::initGLEW();
    bool found_extensions = checkExtensions();
    if (!found_extensions)
        return false;
    
    ogle::Debug::init();

    initGLSettings();

    createParticles();

    createBufferObject();

    return true;
}

void update(){
    BufferMethod[MethodSelection]->update(Particles);
}

void render(){
    glClearColor(0,0,0,0);
    glClear( GL_COLOR_BUFFER_BIT );

    ParticleShader.bind();
    BufferMethod[MethodSelection]->render(Particles);
    ParticleShader.unbind();
}

void runloop(){

    static uint32_t frame_counter = 0;
    static double curr_time = 0;
    static double prev_time = 0;

    bool close = glfwWindowShouldClose(glfwWindow);
    double start, end;
    while (!close){
        // std::cout << "-------------------------------------------------------" << std::endl;
        glfwPollEvents();

        // start = glfwGetTime();
        update();
        // end = glfwGetTime();
        // std::cout << std::setprecision(9) << std::fixed << "Update: " << (end - start) << std::endl;

        render();

        // start = glfwGetTime();
        glfwSwapBuffers(glfwWindow);
        // end = glfwGetTime();
        // std::cout << std::setprecision(9) << std::fixed << "SwapBuffers: " << (end - start) << std::endl;

        close = glfwWindowShouldClose(glfwWindow);
        
        curr_time = glfwGetTime();
        if ( (curr_time-prev_time) > 1.0){
            std::cout << "FPS: " << frame_counter << std::endl;
            frame_counter = 0;
            prev_time = curr_time;
        }
        frame_counter++;
    }
}

void shutdown(){
    for (auto& buff : BufferMethod){
        buff->shutdown();
    }
       
    ParticleShader.shutdown();
    delete [] Particles.VertData;

    ogle::Debug::shutdown();
    
    glfwSetWindowShouldClose(glfwWindow, 1);
    glfwDestroyWindow(glfwWindow);
    glfwTerminate();
}

int main(int argc, char *argv[]){

    if (!init(argc, argv)) {
        std::cerr << "Error occured!" << std::endl;
        shutdown();
        exit( EXIT_FAILURE );
    }

    runloop();

    shutdown();
    exit( EXIT_SUCCESS );
}