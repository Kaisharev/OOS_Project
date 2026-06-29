#pragma once
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "../agent/Agent.hpp"

struct GanttSegment {
        int start, end;
        std::string agent_id;
};

class GanttTracker {
    public:
        explicit GanttTracker (int num_slots);

        void update (int current_tick, const std::vector<std::shared_ptr<Agent>>& slot_agents);

        void close_agent (const std::string& agent_id, int current_tick);

        void print (std::ostream& out) const;

    private:
        int num_slots;
        std::map<int, std::vector<GanttSegment>> gantt;
        std::vector<std::string> last_slot_agent;
};
