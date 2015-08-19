// http://on-demand.gputechconf.com/gtc/2015/video/S5135.html
// look for tokenized rendering for example of how to use @ about 13min.

// tokens for multithreading opt's
/**

    tokenbuffers are normal gl buffers
    don't require gl context
    can be modified from CPU seperate threads (again don't need a gl context)

    complied command list is less flexable then token buffer (slide 18)
    complied command list is the tokenbuffer complied.

    only one gl thread is requried, to make the tokenbuffers? (slide 28)
        and in worker threads the buffers could be sent and filled out
        at the end the buffer are sent bank to the GL thread which emits them to the driver.
        on the gl thread states are captured, what does that mean?

    all rendering is done via FBO's (slide 30)
    can't use uniform's, must be UBO's
        ARB_bindless_texture for textures

    for bindless buffers need to use:
        arb_vertex_attrib_binding
        nv_vertex_buffer_unified_memory

    can no longer "glEnable(...)"

    state capture does not get Scissor info or viewport because they and some other things are controlled by Tokens

    Migration tips on slide 32

    now it is promoted to change matrix heiarchies on gpu using arb_compute_shader (slide 35)

    gpu culling basics slide 37


    tip!! group state changes by frequency of change

    references http://on-demand.gputechconf.com/gtc/2014/presentations/S4379-opengl-44-scene-rendering-techniques.pdf several times
    and http://on-demand.gputechconf.com/siggraph/2014/presentation/SG4117-OpenGL-Scene-Rendering-Techniques.pdf has additional info

    reminder lookup and "get to know" scene tree's (mentioned in the above paper.)
*/



#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <cassert>
#include <map>

#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <gputimer.h>


int main(int argc, char *argv[])
{
    exit( EXIT_SUCCESS );
}