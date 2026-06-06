#include <fstream>
#include <string>
#include <vector>

#include "../Include/nlohmann/json.hpp"
#include "AgentConfig.hpp"
#include "MountPoint.hpp"
#include "SimSettings.hpp"
#pragma once

class Config {
    public:
        SimSettings settings;
        std::vector<MountPoint> mounts;
        std::vector<AgentConfig> agents;

        static Config load_from_file (const std::string& path);

    private:
        void validate () const;
};