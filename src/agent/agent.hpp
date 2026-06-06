#include <Vector>
#include <string>
#include <unordered_map>
#pragma once
#include "../core/AgentState.hpp"
#include "../core/FileHandle.hpp"
#include "Operations/Operations.hpp"

struct Agent {
        std::string id;
        int priority;
        int arrival_time;
        std::vector<Operation> ops;
        int op_index = 0;
        AgentState state = AgentState::READY;

        int start_time = -1;
        int end_time = -1;
        int wait_time = 0;
        int blocked_time = 0;
        int preemptions = 0;

        std::unordered_map<std::string, FileHandle> handles;

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