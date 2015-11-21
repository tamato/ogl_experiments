#ifndef SHADER_PIPELINE
#define SHADER_PIPELINE

#include <map>

namespace ogle
{
    /**
    *   extensions required:
    *   GL_ARB_uniform_buffer_object
    */
    class ProgramObject
    {
    public:
        ProgramObject();
        virtual ~ProgramObject();

        void init( const std::map<unsigned int, std::string>& shaders );

        void bindAttribLoc(unsigned int index, const char * variable);
        void setTexture(unsigned int textureStage, const char * name);
        void setFloat(float val, const char * name);
        void setVec4(const float * vec, const char * name);
        void setVec2(const float * vec, const char * name);
        void setMatrix44(const float * mat, const char * name);

        void bind();
        void unbind();
        void shutdown();

    private:
        void linkProgram();
        unsigned int createShader(unsigned int type, const std::string& filename);
        void collectUniforms();

        unsigned int ProgramName;
        std::map<std::string, int> Uniforms;
    };
}

#endif //SHADER_PIPELINE
