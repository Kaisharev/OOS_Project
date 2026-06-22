#include "DeadlockGraph.hpp"

void DeadlockGraph::add_edge (const std::string& waiter, const std::string& holder) {
    waits_for[waiter] = holder;
}

bool DeadlockGraph::dfs (const std::string& current, const std::string& target, std::set<std::string>& visited) const {
    if (current == target) {
        return true;
    }
    if (visited.find (current) != visited.end ()) {
        return false;
    }
    visited.insert (current);
    auto it = waits_for.find (current);
    if (it != waits_for.end ()) {
        return dfs (it->second, target, visited);
    }
    return false;
}
bool DeadlockGraph::would_create_cycle (const std::string& waiter, const std::string& holder) const {
    std::set<std::string> visited;
    return dfs (holder, waiter, visited);
}

void DeadlockGraph::remove_edges_for (const std::string& agent) {
    waits_for.erase (agent);
}