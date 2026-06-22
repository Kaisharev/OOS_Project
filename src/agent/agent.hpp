#pragma once

#include <Vector>
#include <string>
#include <unordered_map>

#include "../core/AgentState.hpp"
#include "../core/FileHandle.hpp"
#include "Operations.hpp"

class Agent {
    private:
        std::string id;
        int priority;
        int arrival_time;
        int op_index = 0;
        AgentState state = AgentState::READY;

        int start_time = -1;
        int end_time = -1;
        int wait_time = 0;
        int blocked_time = 0;
        int preemptions = 0;

    public:
        std::unordered_map<std::string, FileHandle> handles;
        std::vector<Operation> ops;
        Agent (const std::string& id = "", int priority = 0, int arrival_time = 0)
            : id (id), priority (priority), arrival_time (arrival_time) {}
        bool has_next_op () const {
            return op_index < (int)ops.size ();
        }
        Operation& current_op () {
            return ops[op_index];
        }
        void advance_op () {
            op_index++;
        }
};