#include <iostream>
#include <stdexcept>

#include "VFS/InMemoryVFS.hpp"
#include "agent/Agent.hpp"
#include "agent/Parser/AgentParser.hpp"
#include "core/Config.hpp"

int main () {
    try {
        Config config = Config::load_from_file ("config/json/standard.json");

        for (const auto& agent : config.agents) {
            std::cout << "Agent ID: " << agent.id << ", Priority: " << agent.priority << ", Arrival Time: " << agent.arrival_time
                      << ", Script Path: " << agent.script_path << std::endl;
        }

        for (const auto& mount : config.mounts) {
            std::cout << "Mount Source: " << mount.source << ", Target: " << mount.target
                      << ", Mode: " << (mount.mode == MountPoint::Mode::RO ? "RO" : "RW") << std::endl;
        }
        Agent agent = AgentParser::Parse (config.agents[0]);

        for (const auto& op : agent.ops) {
            std::cout << "Operation Type: " << static_cast<int> (op.getType ()) << ",Duration:" << op.getThinkingDuration ()
                      << ", Path: " << op.getPath () << ", Data: " << op.getData () << ", Mode: " << op.getMode ()
                      << ", Handle: " << op.getHandle () << std::endl;
        }

        InMemoryVFS vfs;
        for (const auto& mount : config.mounts) {
            VFSResult res = vfs.mount (mount);
            if (res != VFSResult::OK) {
                std::cerr << "Failed to mount " << mount.source << " to " << mount.target << ": " << static_cast<int> (res)
                          << std::endl;
            } else {
                std::cout << "Mounted " << mount.source << " to " << mount.target << std::endl;
            }
        }
        vfs.dump (std::cout);
        std::string content;
        vfs.open ("A1", "/shared/jocko.txt", "rw", "f");
        vfs.read ("A1", "f", content);
        std::cout << "Content of file 'f': " << content << std::endl;

        vfs.write ("A1", "f", "new content");
        vfs.read ("A1", "f", content);
        std::cout << "Content of file 'f' after write: " << content << std::endl;

        vfs.open ("A1", "/work/selam.txt", "rw", "f2");
        vfs.append ("A1", "f2", "hello");
        vfs.read ("A1", "f2", content);
        std::cout << "Content of file 'f2' after append: " << content << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what () << std::endl;
        return 1;
    }

    return 0;
}