#include <string>
#include <vector>

#include "../Agent.hpp"
#include "../Operations/Operations.hpp"

class AgentParser {
        /*
            class Operation {
        private:
            OperationType type;
            int thinking_duration;
            std::string path;
            std::string data;
            std::string mode;
            std::string handle;
        }

            THINK <duration> - Operation (THINK, 3,,,,)
            OPEN <path> <mode> as <handle> - Operation (OPEN, ,path,,write,handle)
            READ <handle> - Operation (READ,,,,handle)
            WRITE <handle> <data> - Operation (WRITE,,,data,,handle)
            CLOSE <handle> - Operation (CLOSE,,,,,handle)
            APPEND <handle> <data> - Operation (APPEND,,,data,,handle)

        */

    private:
        std::string script_path;
        std::vector<Operations> ops;
        AgentParser () = delete;

    public:
        Agent Parse (const std::string& script_path);
}