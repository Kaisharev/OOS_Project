#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

class DeadlockGraph {
    public:
        void add_edge (const std::string& waiter, const std::string& holder);
        void remove_edges_for (const std::string& agent);

        // Provjeri bi li NOVI edge waiter->holder stvorio ciklus (bez dodavanja)
        bool would_create_cycle (const std::string& waiter, const std::string& holder) const;
        // Provjeri da li TRENUTNI graf (edge vec dodan) sadrzi ciklus koji ukljucuje waiter
        bool would_create_cycle_after_add (const std::string& waiter) const;

        std::string get_cycle_path (const std::string& waiter, const std::string& holder) const;

    private:
        std::map<std::string, std::string> waits_for;
        bool dfs (const std::string& current, const std::string& target, std::set<std::string>& visited) const;
};
