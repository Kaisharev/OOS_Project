#pragma once

#include <string>
#include <unordered_map>
#include <vector>

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
        std::unordered_map<std::string, FileHandle> handles;
        std::vector<Operation> ops;
        int think_ticks_remaining = 0;

        // other agent internal timers/flags can be added here

    public:
        Agent (const std::string& id = "", int priority = 0, int arrival_time = 0)
            : id (id), priority (priority), arrival_time (arrival_time) {}
        bool has_next_op () const {
            return op_index < (int)ops.size ();
        }
        Operation& current_op () {
            return ops[op_index];
        }
        const Operation& current_op () const {
            return ops[op_index];
        }
        void advance_op () {
            op_index++;
        }
        int getPriority () const {
            return priority;
        }
        void setPriority (int p) {
            priority = p;
        }

        std::string getId () const {
            return id;
        }
        void setId (const std::string& v) {
            id = v;
        }

        int getArrivalTime () const {
            return arrival_time;
        }
        void setArrivalTime (int t) {
            arrival_time = t;
        }

        int getOpIndex () const {
            return op_index;
        }
        void setOpIndex (int idx) {
            op_index = idx;
        }

        AgentState getState () const {
            return state;
        }
        void setState (AgentState s) {
            state = s;
        }

        int getStartTime () const {
            return start_time;
        }
        void setStartTime (int t) {
            start_time = t;
        }

        int getEndTime () const {
            return end_time;
        }
        void setEndTime (int t) {
            end_time = t;
        }

        int getWaitTime () const {
            return wait_time;
        }
        void addWaitTime (int t) {
            wait_time += t;
        }
        void setWaitTime (int t) {
            wait_time = t;
        }

        int getBlockedTime () const {
            return blocked_time;
        }
        void addBlockedTime (int t) {
            blocked_time += t;
        }

        void incrementBlockedTime () { blocked_time++; }

        int getThinkTicksRemaining () const { return think_ticks_remaining; }
        void startThink (int duration) { think_ticks_remaining = duration; }
        void tickThink () { if (think_ticks_remaining > 0) --think_ticks_remaining; }

        int getPreemptions () const {
            return preemptions;
        }
        void incrementPreemptions () {
            preemptions++;
        }

        // handles accessors
        const std::unordered_map<std::string, FileHandle>& getHandles () const {
            return handles;
        }
        void addHandle (const std::string& name, const FileHandle& fh) {
            handles[name] = fh;
        }
        void removeHandle (const std::string& name) {
            handles.erase (name);
        }

        const std::vector<Operation>& getOps () const {
            return ops;
        }
        void setOps (const std::vector<Operation>& v) {
            ops = v;
        }
        void addOp (const Operation& op) {
            ops.push_back (op);
        }
};