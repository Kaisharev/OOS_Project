#pragma once
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "IScheduler.hpp"

class PriorityPreemptiveScheduler : public IScheduler {
    public:
        explicit PriorityPreemptiveScheduler (int max_running_agents);

        void add_agent (std::shared_ptr<Agent> agent) override;
        void tick (int current_time) override;
        std::vector<std::shared_ptr<Agent>> get_running () const override;
        std::vector<std::shared_ptr<Agent>> get_slot_agents () const override;
        void unblock_agent (const std::string& agent_id) override;
        bool all_done () const override;

    private:
        int max_running_agents;

        struct AgentComparator {
            bool operator() (const std::shared_ptr<Agent>& a,
                             const std::shared_ptr<Agent>& b) const {
                return a->getPriority () > b->getPriority ();
            }
        };

        std::vector<std::shared_ptr<Agent>> pending;
        std::priority_queue<std::shared_ptr<Agent>,
                            std::vector<std::shared_ptr<Agent>>,
                            AgentComparator> ready_queue;
        std::vector<std::shared_ptr<Agent>> running_slots;
        std::unordered_map<std::string, std::shared_ptr<Agent>> blocked;

        void fill_slots ();
        void check_preemption ();
        void move_arrived_agents (int current_time);
        void release_done_slots ();
};
