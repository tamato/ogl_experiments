#include "programobject.h"

#include <assert.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>

#define GLEW_NO_GLU
#include <GL/glew.h>

using namespace ogle;

ProgramObject::ProgramObject()
    : ProgramName(0)
{
}

ProgramObject::~ProgramObject()
{
    shutdown();
}

void ProgramObject::init( const std::map<unsigned int, std::string>& shaders )
{
    std::vector<GLuint> shader_list;
    for (const auto& kv: shaders)
        shader_list.push_back( createShader( kv.first, kv.second ) );

    ProgramName = glCreateProgram();
    for (const auto& shader : shader_list)
        glAttachShader(ProgramName, shader);

    linkProgram();
    
    for (const auto& shader : shader_list){
        glDetachShader(ProgramName, shader);
        glDeleteShader(shader);
    }

    collectUniforms();
}

void ProgramObject::bindAttribLoc(GLuint index, const char * variable)
{
    glBindAttribLocation(ProgramName, index, variable);
}

void ProgramObject::setTexture(unsigned int textureStage, const char * name)
{
    glUniform1i(Uniforms[name], textureStage);
}

void ProgramObject::setFloat(float val, const char * name)
{
    glUniform1f(Uniforms[name], val);
}

void ProgramObject::setVec4(const float * vec, const char * name)
{
    glUniform4fv(Uniforms[name], 1, vec);
}

void ProgramObject::setVec2(const float * vec, const char * name)
{
    glUniform2fv(Uniforms[name], 1, vec);    
}

void ProgramObject::setMatrix44(const float * mat, const char * name)
{
    glUniformMatrix4fv(Uniforms[name], 1, GL_FALSE, mat);
}

void ProgramObject::bind()
{
    glUseProgram(ProgramName);
}

void ProgramObject::unbind()
{
    glUseProgram(ProgramName);
}

void ProgramObject::shutdown()
{
    glUseProgram(0);
    glDeleteProgram(ProgramName);
}

void ProgramObject::linkProgram()
{
    // Link Program MUST be done after and bindAttri's
    glLinkProgram(ProgramName);
    int status;
    glGetProgramiv(ProgramName, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        const int buffSize = 1000;
        int length;
        char errorBuffer[buffSize] = {0};
        glGetProgramInfoLog(ProgramName, buffSize, &length, errorBuffer);
        std::cerr << "Shader failed to link: " << "\n\t"
                  << errorBuffer
                  << std::endl;
    }
}

GLuint ProgramObject::createShader(unsigned int type, const std::string& filename)
{
    GLuint shader = glCreateShader(type);

    // load up the file
    std::ifstream inf(filename);
    if (!inf.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        assert(0);
    }

    std::string source;
    inf.seekg(0, std::ios::end);
    source.resize((unsigned int)inf.tellg());
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
        char errorBuffer[maxLen];
        memset((void*)errorBuffer, 0, (size_t)maxLen);
        glGetShaderInfoLog(shader, maxLen, &len, errorBuffer);
        std::cerr   << "Shader: " << "\n\t"
                    << filename << "\n"
                    << "Failed to compile with errors:\n\t"
                    << errorBuffer << std::endl;
        assert(0);
    }

    return shader;
}

void ProgramObject::collectUniforms()
{
    glUseProgram(ProgramName);

    // older way of doing things,
    // upgrade to using uniform buffers...
    int active_uniforms = 0;
    glGetProgramiv(ProgramName, GL_ACTIVE_UNIFORMS, &active_uniforms);

    for (int i=0; i<active_uniforms; ++i) {
        const GLuint len = 50;
        GLsizei used_len = 0;
        GLsizei uniform_size = 0;
        GLenum type;
        char name[len] = {0};
        glGetActiveUniform(ProgramName, GLuint(i), len, &used_len, &uniform_size, &type, name);
        name[used_len] = 0;
        GLint loc = glGetUniformLocation(ProgramName, name);
        Uniforms[std::string(name)] = loc;
        // std::cout << name << " " << loc << std::endl;
    }

    // not using the following because the demos this code is used in knows the location of all the attributes.
    // unsigned int d = glGetAttribLocation(ProgramName, "Position");
    glUseProgram(0);

    // with uniform buffers, found in GL_ARB_uniform_buffer_object
    /*

    //  this example is found in the extensions page at line ~1630.
    //  uniform buffers are buffer objects, to create one use:

    // gets the block index that is within the shader
    // different shader may have different block indices for the same block name. -- gotta verify
    uniformBlockIndex = glGetUniformBlockIndex(prog_id, "colors0");

    // how big to make the buffer
    glGetActiveUniformBlockiv(prog_id, uniformBlockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &uniformBlockSize);
    
    glGenBuffers(1, &buffer_id);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer_id);
    glBufferData(GL_UNIFORM_BUFFER, uniformBlockSize, NULL, GL_DYNAMIC_DRAW);

    // lock buffer_id to a UBO binding point so that we can asscoiate this UBO to a named block in the shader
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, buffer_id);
    glUniformBlockBinding(prog_id, uniformBlockIndex, 0);
    // now buffer_id and uniformBlcokIndex are bound to 0

    // get information about a varible in a block so that we can update its value
    glGetUniformIndices(prog_id, 1, &names[2], &index);                         // using it's name, find the index it has in the block, 1 value will be written into index.
    glGetActiveUniformsiv(prog_id, 1, &index, GL_UNIFORM_OFFSET, &offset);      // read 1 value from index, find the offset of names[2] and store it in offset
    glGetActiveUniformsiv(prog_id, 1, &index, GL_UNIFORM_SIZE, &singleSize);    // now read the size of names[2] so we now how many bytes to pass into our buffer object.

    glGetActiveUniformsiv(uint program,
                          sizei uniformCount,           // number of items in uniformIndices 
                          const uint* uniformIndices,   // active uniforms in the block
                          enum pname,                   // type of information we are looking for, GL_UNIFORM_OFFSET, GL_UNIFORM_SIZE
                          int* params);                 // result of the query

    // update our buffer like we would any buffer. 
    //  This method is safe if we are using the layout std140, if its not std140 then glBufferSubData would need to be used.
    glBufferData(GL_UNIFORM_BUFFER, 80, colors, GL_DYNAMIC_DRAW);
    */
}