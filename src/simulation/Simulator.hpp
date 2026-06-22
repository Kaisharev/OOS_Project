#pragma once
#include <memory>
#include <vector>

#include "../VFS/InMemoryVFS.hpp"
#include "../agent/Agent.hpp"
#include "../core/Config.hpp"
#include "../deadlock/DeadlockGraph.hpp"
#include "../scheduler/IScheduler.hpp"
#include "EventLog.hpp"


class Simulator {
    public:
        explicit Simulator (const Config& cfg);
        void run ();

    private:
        Config cfg;
        std::unique_ptr<IVFS> vfs;
        std::unique_ptr<IScheduler> scheduler;
        DeadlockGraph deadlock_graph;
        EventLog event_log;
        std::vector<std::shared_ptr<Agent>> all_agents;
        int current_tick = 0;

        void init ();
        void step ();
        void execute_operation (std::shared_ptr<Agent> agent);
        void handle_open (std::shared_ptr<Agent> agent, const Operation& op);
        void handle_close (std::shared_ptr<Agent> agent, const Operation& op);
        void try_unblock_agents ();
};