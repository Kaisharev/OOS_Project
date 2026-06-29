#pragma once
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "../agent/Agent.hpp"
#include "../vfs/InMemoryVFS.hpp"
#include "GanttTracker.hpp"

class SimulationReporter {
    public:
        void print_agent_summary (std::ostream& out, const std::vector<std::shared_ptr<Agent>>& agents) const;
        void print_rejected_locks (std::ostream& out, const std::vector<std::string>& rejected_locks) const;
        void print_statistics (std::ostream& out, const std::vector<std::shared_ptr<Agent>>& agents,
                               const std::vector<std::string>& rejected_locks) const;
};
