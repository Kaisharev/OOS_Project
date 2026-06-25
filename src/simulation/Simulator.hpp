#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../VFS/InMemoryVFS.hpp"
#include "../agent/Agent.hpp"
#include "../core/Config.hpp"
#include "../deadlock/DeadlockGraph.hpp"
#include "../scheduler/IScheduler.hpp"
#include "EventLog.hpp"

// Jedan segment Gantove karte: koji agent (prazan string = idle) i u kom intervalu
struct GanttSegment {
    int start;
    int end;
    std::string agent_id;  // prazan = idle
};

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

        // Gantova karta: slot_index -> lista segmenata
        std::map<int, std::vector<GanttSegment>> gantt;
        // Za pracenje sta je u svakom slotu proslog tika
        std::vector<std::string> last_slot_agent;
        // Odbijeni pokusaji zakljucavanja radi deadlock prevencije
        std::vector<std::string> rejected_locks;

        void init ();
        void step ();
        void execute_operation (std::shared_ptr<Agent> agent);
        void handle_open (std::shared_ptr<Agent> agent, const Operation& op);
        void handle_close (std::shared_ptr<Agent> agent, const Operation& op);
        void try_unblock_agents ();
        void update_gantt ();
        void update_wait_times ();

        void print_gantt (std::ostream& out) const;
        void print_agent_summary (std::ostream& out) const;
        void print_rejected_locks (std::ostream& out) const;
        void print_statistics (std::ostream& out) const;
};
