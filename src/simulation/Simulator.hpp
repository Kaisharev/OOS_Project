#pragma once
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "../agent/Agent.hpp"
#include "../core/Config.hpp"
#include "../deadlock/DeadlockGraph.hpp"
#include "../execution/OperationExecutor.hpp"
#include "../reporting/GanttTracker.hpp"
#include "../reporting/SimulationReporter.hpp"
#include "../scheduler/IScheduler.hpp"
#include "../vfs/InMemoryVFS.hpp"
#include "EventLog.hpp"

class Simulator {
    public:
        explicit Simulator (const Config& cfg);
        void run ();

    private:
        Config cfg;
        int current_tick = 1;

        std::unique_ptr<InMemoryVFS> vfs;
        std::unique_ptr<IScheduler> scheduler;
        std::vector<std::shared_ptr<Agent>> all_agents;

        DeadlockGraph deadlock_graph;
        EventLog event_log;
        std::vector<std::string> rejected_locks;

        std::unique_ptr<OperationExecutor> executor;
        std::unique_ptr<GanttTracker> gantt_tracker;
        SimulationReporter reporter;

        void init ();
        void step ();
        void try_unblock_agents ();
        void update_wait_times ();
        bool is_global_deadlock () const;
        void mark_done (std::shared_ptr<Agent> agent);  // zatvara Gantt segment
};
