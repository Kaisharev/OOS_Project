#pragma once
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


class DeadlockGraph {
    public:
        void add_edge (const std::string& waiter, const std::string& holder);
        void remove_edges_for (const std::string& agent);
        bool would_create_cycle (const std::string& waiter, const std::string& holder) const;
        std::string get_cycle_path (const std::string& waiter, const std::string& holder) const;

    private:
        std::unordered_map<std::string, std::string> waits_for;

        bool dfs (const std::string& current, const std::string& target, std::set<std::string>& visited) const;
};