#ifndef GPUTIMER_H
#define GPUTIMER_H

/****************************************************************

    Uses OpenGL GL_ARB_timer_query to do performance testing.

    Current implementation does not use Sync Object's,
    and it is expected that performance testing is not
    100% accurate.

****************************************************************/

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
