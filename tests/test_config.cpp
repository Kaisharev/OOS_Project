#include <gtest/gtest.h>

#include "../src/core/Config.hpp"

TEST (ConfigTest, Parsiranje) {
    auto cfg = Config::load_from_file ("../config/json/standard.json");

    EXPECT_EQ (cfg.agents.size (), 3);
    EXPECT_EQ (cfg.settings.max_running_agents, 3);
    EXPECT_EQ (cfg.mounts[0].mode, MountPoint::Mode::RO);
}