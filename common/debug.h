/**
    Helper class to help with getting OpenGL debug information
*/
#include <map>
#include <string>
namespace ogle {

    class Debug
    {
    public:
        static void init();
        static const std::string& sourceError(unsigned int);
        static const std::string& typeError(unsigned int);
        static const std::string& idError(unsigned int);
        static const std::string& severityError(unsigned int);
        static void shutdown();

    private:
        struct State
        {
            void init();
            std::map<unsigned int, std::string> Source;
            std::map<unsigned int, std::string> Type;
            std::map<unsigned int, std::string> IDs;
            std::map<unsigned int, std::string> Severity;
        };

        static State* Instance;
        static State* instance();
        static bool InstanceDestroyed;

        Debug();
        ~Debug();
        Debug(const Debug& other);
        Debug& operator=(const Debug& other);
    };
}