#include "Config.hpp"

#include <iostream>

Config Config::load_from_file (const std::string& path) {
    std::ifstream file (path);
    if (!file.is_open ()) {
        throw std::runtime_error ("Could not open config file: " + path);
    }
    nlohmann::json j;
    nlohmann::json data = nlohmann::json::parse (file);
    Config config;
    config.settings.max_running_agents = data["settings"].value ("max_running_agents", 2);

    std::cout << "Loaded config: max_running_agents = " << config.settings.max_running_agents << std::endl;

    for (auto& mount : data["vfs"]["mounts"]) {
        MountPoint mp;
        mp.source = mount["source"].get<std::string> ();
        mp.target = mount["target"].get<std::string> ();
        const std::string mode = mount["mode"].get<std::string> ();

        if (mode == "ro") {
            mp.mode = MountPoint::Mode::RO;
        } else if (mode == "readwrite" || mode == "rw") {
            mp.mode = MountPoint::Mode::RW;
        } else {
            throw std::runtime_error ("Nepoznat mode: " + mount["mode"].get<std::string> ());
        }
        config.mounts.push_back (mp);
    }

    for (auto& agent : data["agents"]) {
        AgentConfig ac;
        ac.id = agent["id"].get<std::string> ();
        ac.priority = agent["priority"].get<int> ();
        ac.arrival_time = agent["arrival_time"].get<int> ();
        ac.script_path = agent["script"].get<std::string> ();
        config.agents.push_back (ac);
    }
    config.validate ();
    return config;
}

void Config::validate () const {
    if (settings.max_running_agents <= 0) {
        throw std::runtime_error ("max_running_agents must be greater than 0");
    }
    for (const auto& mount : mounts) {
        if (mount.source.empty () || mount.target.empty ()) {
            throw std::runtime_error ("Mount points must have non-empty source and target");
        }
    }
    for (const auto& agent : agents) {
        if (agent.id.empty () || agent.script_path.empty ()) {
            throw std::runtime_error ("Agents must have non-empty id and script_path");
        }
    }
}