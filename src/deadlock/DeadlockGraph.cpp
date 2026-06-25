#include "DeadlockGraph.hpp"

void DeadlockGraph::add_edge (const std::string& waiter, const std::string& holder) {
    waits_for[waiter] = holder;
}

void DeadlockGraph::remove_edges_for (const std::string& agent) {
    waits_for.erase (agent);
}

bool DeadlockGraph::dfs (const std::string& current, const std::string& target,
                         std::set<std::string>& visited) const {
    if (current == target) return true;
    if (visited.count (current)) return false;
    visited.insert (current);
    auto it = waits_for.find (current);
    if (it != waits_for.end ())
        return dfs (it->second, target, visited);
    return false;
}

// Provjeri bi li dodavanje edge waiter->holder stvorilo ciklus (edge JOS nije u grafu)
bool DeadlockGraph::would_create_cycle (const std::string& waiter, const std::string& holder) const {
    if (waiter == holder) return true;
    std::set<std::string> visited;
    return dfs (holder, waiter, visited);
}

// Provjeri da li vec postoji ciklus koji ukljucuje 'waiter' (edge JE vec dodan)
bool DeadlockGraph::would_create_cycle_after_add (const std::string& waiter) const {
    std::set<std::string> visited;
    return dfs (waiter, waiter, visited);
}

std::string DeadlockGraph::get_cycle_path (const std::string& waiter,
                                            const std::string& holder) const {
    std::vector<std::string> path;
    std::set<std::string> visited;
    path.push_back (waiter);
    std::string current = holder;
    while (true) {
        path.push_back (current);
        if (current == waiter) break;
        if (visited.count (current)) break;
        visited.insert (current);
        auto it = waits_for.find (current);
        if (it == waits_for.end ()) break;
        current = it->second;
    }
    std::string result;
    for (size_t i = 0; i < path.size (); i++) {
        result += path[i];
        if (i + 1 < path.size ()) result += " -> ";
    }
    return result;
}
