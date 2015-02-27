#include "common.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {

}

namespace ogle {

    Framebuffer::Framebuffer()
        : FramebufferName(0)
        , Target(0)
        , ComponentCount(0)
        , InternalFormat(0)
        , Width(0)
        , Height(0)
        , Format(0)
        , Type(0)
    {

    }

    void Framebuffer::shutdown()
    {
        glDeleteTextures(TextureNames.size(), TextureNames.data());
        glDeleteFramebuffers(1, &FramebufferName);
    }

    ShaderProgram::ShaderProgram() : ProgramName(0) {}
    ShaderProgram::~ShaderProgram() {
        shutdown();
    }

    void ShaderProgram::init( const std::map<GLuint, std::string>& shaders )
    {
        std::vector<GLuint> shader_list;
        for (const auto& kv: shaders)
            shader_list.push_back( createShader( kv.first, kv.second ) );

        ProgramName = glCreateProgram();
        for (const auto& shader : shader_list)
            glAttachShader(ProgramName, shader);

        glLinkProgram(ProgramName);

        for (const auto& shader : shader_list)
            glDeleteShader(shader);

        checkShaderLinkage(ProgramName);
        glUseProgram(ProgramName);
        collectUniforms();
        glUseProgram(0);
    }

    void ShaderProgram::bind()
    {
        if (0 == ProgramName){
            std::string msg = "Tried using shader program object that was not valid. ";
            msg += __FILE__;
            msg += " : ";
            msg += __LINE__;
            throw std::runtime_error(msg);
        }
        glUseProgram(ProgramName);
    }

    void ShaderProgram::collectUniforms()
    {
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
    }

    void ShaderProgram::shutdown()
    {
        // the ProgramName is stored in an array and is managed by someone else
        // some sort of communication needs to be set up to notify each other for shutdowns.
        ProgramName = 0;
    }

    GLuint ShaderProgram::createShader(GLenum type, const std::string& filename)
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

    void ShaderProgram::checkShaderLinkage( const GLuint& program)
    {
        int status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            const int maxLen = 1000;
			int len;
			char errorBuffer[maxLen];
			memset((void*)errorBuffer, 0, (size_t)maxLen);
            glGetProgramInfoLog(program, maxLen, &len, errorBuffer);
            std::cerr   << "Shader Linked with erros:\n"
                        << errorBuffer << std::endl;
            assert(0);
        }
    }

    FullscreenQuad::FullscreenQuad()
        : VertCount(4)
        , ByteCount(VertCount * sizeof(glm::vec2))
	{
		Verts.push_back(glm::vec2(-1,-1));
		Verts.push_back(glm::vec2( 1,-1));
		Verts.push_back(glm::vec2( 1, 1));
		Verts.push_back(glm::vec2(-1, 1));
    }

    FullscreenQuad::~FullscreenQuad()
    {
        shutdown();
    }

    void FullscreenQuad::init()
    {
        glGenVertexArrays(1, &VAO_Name);
        glGenBuffers(1, &BufferName);

        glBindVertexArray(VAO_Name);
        glEnableVertexAttribArray(0); // positions

        glBindBuffer(GL_ARRAY_BUFFER, BufferName);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glBindBuffer(GL_ARRAY_BUFFER, BufferName);
        glBufferData(GL_ARRAY_BUFFER, ByteCount, glm::value_ptr(Verts[0]), GL_STATIC_DRAW);
    }

    void FullscreenQuad::render()
    {
        glBindVertexArray(VAO_Name);
        glDrawArrays(GL_TRIANGLE_FAN, 0, VertCount);
    }

    void FullscreenQuad::shutdown()
    {
        glDeleteBuffers(1, &BufferName);
        glDeleteVertexArrays(1, &VAO_Name);
    }

    GLuint initTexture(GLenum target, GLint internalFormat, GLuint componentCount, GLsizei width, GLsizei height, GLenum format, GLenum type)
    {
        GLuint textureName;
        glGenTextures(1, &textureName);
        glBindTexture(target, textureName);

        unsigned int size = width * height * componentCount;
        unsigned int *data = new unsigned int[size];
        memset((void*)data, 0, size*sizeof(unsigned int));

        // Nvidia has a bug that if these were intergal textures
        // GL_LINEAR cannot be used and must be GL_NEAREST
        glTexParameteri( target, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri( target, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        glTexParameteri( target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        glTexImage2D(
            target, 0,
            internalFormat,
            width,
            height,
            0,
            format,
            type,
            data
        );

        if (glGetError() != GL_NONE) assert(0);

        glBindTexture(target, 0);
        delete [] data;
        return textureName;
    }


    void initFramebuffer(Framebuffer& framebuffer)
    {
        for (auto& texture : framebuffer.TextureNames) {
            texture = initTexture(
                framebuffer.Target,
                framebuffer.InternalFormat,
                framebuffer.ComponentCount,
                framebuffer.Width,
                framebuffer.Height,
                framebuffer.Format,
                framebuffer.Type
                );
        }

        glGenFramebuffers(1, &framebuffer.FramebufferName);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.FramebufferName);

        size_t buffer_count = framebuffer.TextureNames.size();
        std::vector<GLenum> draw_buffers(buffer_count);
        for (size_t i=0; i<framebuffer.TextureNames.size(); ++i){
            glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, framebuffer.Target, framebuffer.TextureNames[i], 0);
            draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        }

        glDrawBuffers(buffer_count, draw_buffers.data());

        // check for completeness
        ogle::checkFramebufferStatus();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void checkFramebufferStatus()
    {
        GLenum result = glCheckFramebufferStatus( GL_FRAMEBUFFER );
        if (result != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "FBO incomplete, error: " << std::endl;
            switch (result)
            {
                /*********************************************************************
                  These values were found from:
                    http://www.opengl.org/wiki/GLAPI/glCheckFramebufferStatus
                *********************************************************************/
                case GL_FRAMEBUFFER_UNDEFINED:
                    std::cout << "\tGL_FRAMEBUFFER_UNDEFINED\n";
                    std::cout << "\t-target- is the default framebuffer";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT\n";
                    std::cout << "\tthe framebuffer attachment is incomplete";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT\n";
                    std::cout << "\tthere are no images attached to the framebuffer";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER\n";
                    std::cout << "\tone of the color attaches has an object type of GL_NONE";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER\n";
                    std::cout << "\tGL_READ_BUFFER attached object type of GL_NONE";
                    break;
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    std::cout << "\tGL_FRAMEBUFFER_UNSUPPORTED\n";
                    std::cout << "\tinternal formats conflict with implementation-dependent restrictions";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE\n";
                    std::cout << "\tis also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS"
                         << "\tis not the same for all attached textures; or, if the attached images"
                         << "\tare a mix of renderbuffers and textures, the value of"
                         << "\tGL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.";
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
                    std::cout << "\tGL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS\n";
                    std::cout << "\ta framebuffer attachment is layered, and a populated attachment is not layered,"
                         << "\tor if all populated color attachments are not from textures of the same target.";
                    break;
                default:
                    std::cout << "\tUnknown error occured.";
            }
            std::cout << std::endl;
            assert(0);
        }
    }

    void   initGLEW()
    {

    }

    void   setDataDir(int argc, char *argv[])
    {

    }
}
