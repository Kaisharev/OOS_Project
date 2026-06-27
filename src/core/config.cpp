#include "Config.hpp"

#include <iostream>

Config Config::load_from_file (const std::string& path) {
    std::ifstream file (path);
    if (!file.is_open ()) {
        throw std::runtime_error ("Ne mogu otvoriti config fajl: " + path);
    }

    nlohmann::json data;
    try {
        data = nlohmann::json::parse (file);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error ("JSON parse greska u '" + path + "': " + std::string (e.what ()));
    }

    Config config;

    try {
        config.settings.max_running_agents = data.at ("settings").value ("max_running_agents", 2);
    } catch (const std::exception& e) {
        throw std::runtime_error ("Greska pri citanju 'settings': " + std::string (e.what ()));
    }

    std::cout << "Loaded config: max_running_agents = " << config.settings.max_running_agents << std::endl;

    try {
        for (auto& mount : data.at ("vfs").at ("mounts")) {
            MountPoint mp;
            mp.source = mount.at ("source").get<std::string> ();
            mp.target = mount.at ("target").get<std::string> ();
            const std::string mode = mount.at ("mode").get<std::string> ();

            if (mode == "ro") {
                mp.mode = MountPoint::Mode::RO;
            } else if (mode == "rw" || mode == "readwrite") {
                mp.mode = MountPoint::Mode::RW;
            } else {
                throw std::runtime_error ("Nepoznat mount mode: '" + mode + "'");
            }
            config.mounts.push_back (mp);
        }
    } catch (const std::runtime_error&) {
        throw;  // proslijedi nas exception dalje
    } catch (const std::exception& e) {
        throw std::runtime_error ("Greska pri citanju 'vfs.mounts': " + std::string (e.what ()));
    }

    try {
        for (auto& agent : data.at ("agents")) {
            AgentConfig ac;
            ac.id = agent.at ("id").get<std::string> ();
            ac.priority = agent.at ("priority").get<int> ();
            ac.arrival_time = agent.at ("arrival_time").get<int> ();
            ac.script_path = agent.at ("script").get<std::string> ();
            config.agents.push_back (ac);
        }
    } catch (const std::runtime_error&) {
        throw;
    } catch (const std::exception& e) {
        throw std::runtime_error ("Greska pri citanju 'agents': " + std::string (e.what ()));
    }

    config.validate ();
    return config;
}

void Config::validate () const {
    if (settings.max_running_agents <= 0) {
        throw std::runtime_error ("max_running_agents mora biti veci od 0");
    }
    for (const auto& mount : mounts) {
        if (mount.source.empty () || mount.target.empty ()) {
            throw std::runtime_error ("Mount mora imati neprazan source i target");
        }
    }
    for (const auto& agent : agents) {
        if (agent.id.empty () || agent.script_path.empty ()) {
            throw std::runtime_error ("Agent '" + agent.id + "' mora imati neprazan id i script path");
        }
    }
}
