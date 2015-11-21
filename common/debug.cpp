#include "debug.h"

#include <iostream>

#define GLEW_NO_GLU
#include <GL/glew.h>

using namespace std;
using namespace ogle;

Debug::State* Debug::Instance = 0;
Debug::State* Debug::instance()
{
    if (!InstanceDestroyed && !Instance)
    {
        Instance = new State;
        Instance->init();
    }
    return Instance;
}

bool Debug::InstanceDestroyed = false;

void debugOutput(
    unsigned int source,
    unsigned int type,
    unsigned int id,
    unsigned int severity,
    int length,
    const char* message,
    void* userParam)
{

    cout << "OGL Debugger Error: \n"
         << "\tSource: "    << Debug::sourceError(source) << "\n"
         << "\tType: "      << Debug::typeError(type) << "\n"
         << "\tID: "        << Debug::idError(id) << "\n"
         << "\tSeverity: "  << Debug::severityError(severity) << "\n"
         << "\tMessage: "   << message
         << endl;
}

void Debug::init()
{
    if (State *i = instance())
    {
        i->init();
    }
}

const string NotKnown = "ID for error not known";
const string& Debug::sourceError(unsigned int id)
{
    if (State *i = instance())
    {
        return i->Source[id];
    }
    return NotKnown;
}

const string& Debug::typeError(unsigned int id)
{
    if (State *i = instance())
    {
        return i->Type[id];
    }
    return NotKnown;
}

const string& Debug::idError(unsigned int id)
{
    if (State *i = instance())
    {
        return i->IDs[id];
    }
    return NotKnown;
}

const string& Debug::severityError(unsigned int id)
{
    if (State *i = instance())
    {
        return i->Severity[id];
    }
    return NotKnown;
}

void Debug::shutdown()
{
    delete Instance;
    Instance = nullptr;
    InstanceDestroyed = true;
}

void Debug::State::init()
{    
    #define ARRAY(arr, val) arr[val] = #val
    ARRAY(Source, GL_DEBUG_SOURCE_API_ARB);
    ARRAY(Source, GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB);
    ARRAY(Source, GL_DEBUG_SOURCE_SHADER_COMPILER_ARB);
    ARRAY(Source, GL_DEBUG_SOURCE_THIRD_PARTY_ARB);
    ARRAY(Source, GL_DEBUG_SOURCE_APPLICATION_ARB);
    ARRAY(Source, GL_DEBUG_SOURCE_OTHER_ARB);

    ARRAY(Type, GL_DEBUG_TYPE_ERROR_ARB);
    ARRAY(Type, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB);
    ARRAY(Type, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB);
    ARRAY(Type, GL_DEBUG_TYPE_PORTABILITY_ARB);
    ARRAY(Type, GL_DEBUG_TYPE_PERFORMANCE_ARB);
    ARRAY(Type, GL_DEBUG_TYPE_OTHER_ARB);

    ARRAY(Severity, GL_DEBUG_SEVERITY_HIGH_ARB);
    ARRAY(Severity, GL_DEBUG_SEVERITY_MEDIUM_ARB);
    ARRAY(Severity, GL_DEBUG_SEVERITY_LOW_ARB);

    /** General OpenGL Errors */
    ARRAY(IDs, GL_INVALID_ENUM);
    ARRAY(IDs, GL_INVALID_VALUE);
    ARRAY(IDs, GL_INVALID_OPERATION);
    ARRAY(IDs, GL_STACK_OVERFLOW);
    ARRAY(IDs, GL_STACK_UNDERFLOW);
    ARRAY(IDs, GL_OUT_OF_MEMORY);

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    glDebugMessageCallbackARB((GLDEBUGPROCARB)debugOutput, nullptr);
}

Debug::Debug()
{

}

Debug::~Debug()
{

}

Debug::Debug(const Debug& other)
{

}

Debug& Debug::operator=(const Debug& other)
{
    return *this;
}
