#ifndef GPUTIMER_H
#define GPUTIMER_H

#include <vector>
#include <GL/glew.h>

namespace sp
{
    class gpuTimer
    {
    public:
        gpuTimer();
        ~gpuTimer();

        void init();
        void start();
        void end();
        /** returns a value in milliseconds */
        double elaspedTime();

        int activeQueries();

    private:
        struct gpuQueryTimer
        {
            GLuint start;
            GLuint end;
            bool in_use;
        };

        GLsync sync;
        GLuint activeIdx;
        std::vector<gpuQueryTimer> queries;
    };
}
#endif // GPUTIMER_H
