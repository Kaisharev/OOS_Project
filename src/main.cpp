#include <iostream>
#include <stdexcept>

#include "agent/Agent.hpp"
#include "agent/Parser/AgentParser.hpp"
#include "core/Config.hpp"

int main () {
    try {
        Config config = Config::load_from_file ("config/json/standard.json");

        // Initialize and run the simulation using the loaded config

        for (const auto& agent : config.agents) {
            std::cout << "Agent ID: " << agent.id << ", Priority: " << agent.priority << ", Arrival Time: " << agent.arrival_time
                      << ", Script Path: " << agent.script_path << std::endl;
        }

        for (const auto& mount : config.mounts) {
            std::cout << "Mount Source: " << mount.source << ", Target: " << mount.target
                      << ", Mode: " << (mount.mode == MountPoint::Mode::RO ? "RO" : "RW") << std::endl;
        }
        Agent agent = AgentParser::Parse (std::string (config.agents[0].script_path));

        for (const auto& op : agent.ops) {
            std::cout << "Operation Type: " << static_cast<int> (op.getType ()) << ", Path: " << op.getPath ()
                      << ", Data: " << op.getData () << ", Mode: " << op.getMode () << ", Handle: " << op.getHandle () << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what () << std::endl;
        return 1;
    }

    return 0;
}