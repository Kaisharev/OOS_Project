#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../agent/Agent.hpp"

class IScheduler {
    public:
        virtual ~IScheduler () = default;

        virtual void add_agent (std::shared_ptr<Agent> agent) = 0;

        virtual void tick (int current_time) = 0;

        virtual std::vector<std::shared_ptr<Agent>> get_running () const = 0;

        virtual void unblock_agent (const std::string& agent_id) = 0;

        virtual bool all_done () const = 0;
};