#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../agent/Agent.hpp"
#include "../deadlock/DeadlockGraph.hpp"
#include "../simulation/EventLog.hpp"
#include "../vfs/InMemoryVFS.hpp"

// SRP: iskljucivo odgovoran za izvrsavanje jedne operacije jednog agenta.
// OCP: dodavanje nove operacije = nova metoda handle_X, bez diranja postojecih.
class OperationExecutor {
    public:
        OperationExecutor (InMemoryVFS& vfs, EventLog& event_log, DeadlockGraph& deadlock_graph,
                           std::vector<std::string>& rejected_locks);

        // Izvrsava trenutnu operaciju agenta.
        // Vraca true ako je operacija potrosila tik (THINK), false ako se moze nastaviti.
        bool execute (std::shared_ptr<Agent> agent, int current_tick);

        // Pokusava deblokirat sve blokirane agente (zove se nakon CLOSE).
        void try_unblock_agents (const std::vector<std::shared_ptr<Agent>>& all_agents,
                                 std::function<void (const std::string&)> unblock_cb, int current_tick);

    private:
        InMemoryVFS& vfs;
        EventLog& event_log;
        DeadlockGraph& deadlock_graph;
        std::vector<std::string>& rejected_locks;

        void handle_think (std::shared_ptr<Agent> agent, int current_tick);
        void handle_read (std::shared_ptr<Agent> agent, int current_tick);
        void handle_write (std::shared_ptr<Agent> agent, int current_tick);
        void handle_append (std::shared_ptr<Agent> agent, int current_tick);
        void handle_open (std::shared_ptr<Agent> agent, int current_tick);
        void handle_close (std::shared_ptr<Agent> agent, int current_tick,
                           std::function<void (const std::string&)> unblock_cb);

        void mark_done (std::shared_ptr<Agent> agent, int current_tick);
};
