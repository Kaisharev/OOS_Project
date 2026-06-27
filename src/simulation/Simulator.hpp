#pragma once
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "../agent/Agent.hpp"
#include "../core/Config.hpp"
#include "../deadlock/DeadlockGraph.hpp"
#include "../scheduler/IScheduler.hpp"
#include "../vfs/InMemoryVFS.hpp"
#include "EventLog.hpp"

class Simulator {
    public:
        explicit Simulator (const Config& cfg);
        void run ();

    private:
        Config cfg;
        int current_tick = 1;  // tick 0 je rezervisan za arrival/inicijalizaciju

        std::unique_ptr<InMemoryVFS> vfs;
        std::unique_ptr<IScheduler> scheduler;
        std::vector<std::shared_ptr<Agent>> all_agents;

        DeadlockGraph deadlock_graph;
        EventLog event_log;
        std::vector<std::string> rejected_locks;

        struct GanttSegment {
                int start, end;
                std::string agent_id;
        };
        std::map<int, std::vector<GanttSegment>> gantt;
        std::vector<std::string> last_slot_agent;

        void init ();
        void step ();
        void execute_operation (std::shared_ptr<Agent> agent);
        void handle_open (std::shared_ptr<Agent> agent, const Operation& op);
        void handle_close (std::shared_ptr<Agent> agent, const Operation& op);
        void try_unblock_agents ();
        void update_gantt ();
        void update_wait_times ();
        bool is_global_deadlock () const;

        void print_gantt (std::ostream& out) const;
        void print_agent_summary (std::ostream& out) const;
        void print_rejected_locks (std::ostream& out) const;
        void print_statistics (std::ostream& out) const;
        void mark_done (std::shared_ptr<Agent> agent);
        void close_gantt_for_agent (const std::string& agent_id);
        void execute_agent_tick (std::shared_ptr<Agent> agent);
};
