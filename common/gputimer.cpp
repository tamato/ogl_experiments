#include "gputimer.h"

#include <iostream>

using namespace ogle;

gpuTimer::gpuTimer()
{

}

gpuTimer::~gpuTimer()
{
    for (size_t i=0; i<queries.size(); ++i) {
        glDeleteQueries(2, &queries[i].start);
    }
}

void gpuTimer::init()
{
    const int default_size = 5;
    queries.reserve(default_size);

    const int default_2 = default_size * 2;
    GLuint q[default_2];
    glGenQueries(default_2, q);
    for (size_t i=0; i<default_size; ++i) {
        gpuQueryTimer qt;
        qt.start = q[i * 2 + 0];
        qt.end   = q[i * 2 + 1];
        qt.in_use = false;
        queries.push_back(qt);
    }
}

void gpuTimer::start()
{
    activeIdx = queries.size();
    for (size_t i=0; i<queries.size(); ++i) {
        if (queries[i].in_use == false) {
            activeIdx = i;
            queries[i].in_use = true;
            break;
        }
    }

    // if no available queries were found, make some.
    if (activeIdx == queries.size()) {
        GLuint q[2];
        glGenQueries(2, q);

        gpuQueryTimer qt;
        qt.start = q[0];
        qt.end = q[1];
        qt.in_use = true;
        queries.push_back(qt);
    }

    glQueryCounter(queries[activeIdx].start, GL_TIMESTAMP);
}

void gpuTimer::end()
{
    glQueryCounter(queries[activeIdx].end, GL_TIMESTAMP);
    sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // now wait for all the gpu commands to clear out
    glFlush();  // must call this manually when using waitsync
    glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
    glDeleteSync(sync);
}

double gpuTimer::elaspedTime()
{
    GLint start_available;
    GLint end_available;

    GLuint64 timer_start;
    GLuint64 timer_end;
    GLuint64 timer_elapsed;

    // return a negative value when no query objects are available
    double time = 0.0;

    for (size_t i=0; i<queries.size(); ++i) {
        if (queries[i].in_use == true) {

            while(end_available){
                glGetQueryObjectiv(queries[i].end,   GL_QUERY_RESULT_AVAILABLE, &end_available);
            }
            
            // See how much time the rendering of object i took in nanoseconds.
            glGetQueryObjectui64v(queries[i].start, GL_QUERY_RESULT, &timer_start);
            glGetQueryObjectui64v(queries[i].end,   GL_QUERY_RESULT, &timer_end);

            // getting the difference
            timer_elapsed = timer_end - timer_start;
            time = timer_elapsed * 1e-6f; // convert into milliseconds

            // clear values to be used again
            queries[i].in_use = false;

            // we got a time, don't bother with the rest
            break;
        }
    }

    return time;
}

int gpuTimer::activeQueries()
{
    int result = 0;
    for (size_t i=0; i<queries.size(); ++i){
        if (queries[i].in_use == true) {
            result++;
        }
    }
    return result;
}
