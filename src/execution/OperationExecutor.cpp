#include "OperationExecutor.hpp"

#include <functional>

OperationExecutor::OperationExecutor (InMemoryVFS& vfs, EventLog& event_log, DeadlockGraph& deadlock_graph,
                                      std::vector<std::string>& rejected_locks)
    : vfs (vfs), event_log (event_log), deadlock_graph (deadlock_graph), rejected_locks (rejected_locks) {}

bool OperationExecutor::execute (std::shared_ptr<Agent> agent, int current_tick) {
    if (!agent->has_next_op ()) return false;
    const Operation& op = agent->current_op ();
    switch (op.getType ()) {
        case OperationType::THINK:
            handle_think (agent, current_tick);
            return true;  // THINK uvijek trosi tik
        case OperationType::READ:
            handle_read (agent, current_tick);
            return false;
        case OperationType::WRITE:
            handle_write (agent, current_tick);
            return false;
        case OperationType::APPEND:
            handle_append (agent, current_tick);
            return false;
        case OperationType::OPEN:
            handle_open (agent, current_tick);
            return false;
        case OperationType::CLOSE:
            handle_close (agent, current_tick, nullptr);
            return false;
    }
    return false;
}

void OperationExecutor::try_unblock_agents (const std::vector<std::shared_ptr<Agent>>& all_agents,
                                            std::function<void (const std::string&)> unblock_cb, int current_tick) {
    for (auto& agent : all_agents) {
        if (agent->getState () != AgentState::BLOCKED) continue;
        if (!agent->has_next_op ()) continue;
        const Operation& op = agent->current_op ();
        if (op.getType () != OperationType::OPEN) continue;
        std::string holder = vfs.get_lock_holder (op.getPath ());
        if (!holder.empty ()) continue;
        deadlock_graph.remove_edges_for (agent->getId ());
        if (unblock_cb) unblock_cb (agent->getId ());
        event_log.log (current_tick, agent->getId () + " deblokiran, ponavlja OPEN " + op.getPath ());
        execute (agent, current_tick);
    }
}

void OperationExecutor::mark_done (std::shared_ptr<Agent> agent, int current_tick) {
    agent->setState (AgentState::DONE);
    agent->setEndTime (current_tick);
    event_log.log (current_tick, "Agent " + agent->getId () + " zavrsio");
}

void OperationExecutor::handle_think (std::shared_ptr<Agent> agent, int current_tick) {
    const Operation& op = agent->current_op ();
    if (agent->getThinkTicksRemaining () == 0) {
        agent->startThink (op.getThinkingDuration ());
        event_log.log (current_tick, agent->getId () + " THINK " + std::to_string (op.getThinkingDuration ()));
    }
    agent->tickThink ();
    if (agent->getThinkTicksRemaining () == 0) {
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
    }
}

void OperationExecutor::handle_read (std::shared_ptr<Agent> agent, int current_tick) {
    const Operation& op = agent->current_op ();
    std::string content;
    VFSResult res = vfs.read (agent->getId (), op.getHandle (), content);
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> \"" + content + "\"");
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
    } else {
        event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> greska");
    }
}

void OperationExecutor::handle_write (std::shared_ptr<Agent> agent, int current_tick) {
    const Operation& op = agent->current_op ();
    VFSResult res = vfs.write (agent->getId (), op.getHandle (), op.getData ());
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " WRITE " + op.getHandle ());
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
    }
}

void OperationExecutor::handle_append (std::shared_ptr<Agent> agent, int current_tick) {
    const Operation& op = agent->current_op ();
    VFSResult res = vfs.append (agent->getId (), op.getHandle (), op.getData ());
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " APPEND " + op.getHandle () + " \"" + op.getData () + "\"");
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
    }
}

void OperationExecutor::handle_open (std::shared_ptr<Agent> agent, int current_tick) {
    const Operation& op = agent->current_op ();
    VFSResult res = vfs.open (agent->getId (), op.getPath (), op.getMode (), op.getHandle ());

    if (res == VFSResult::OK) {
        event_log.log (current_tick,
                       agent->getId () + " OPEN " + op.getPath () + " " + op.getMode () + " as " + op.getHandle () + " -> zakljucano");
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
        return;
    }

    if (res == VFSResult::WOULD_BLOCK) {
        std::string holder = vfs.get_lock_holder (op.getPath ());

        if (!holder.empty ()) deadlock_graph.add_edge (agent->getId (), holder);

        if (!holder.empty () && deadlock_graph.would_create_cycle_after_add (agent->getId ())) {
            deadlock_graph.remove_edges_for (agent->getId ());
            std::string cycle = deadlock_graph.get_cycle_path (agent->getId (), holder);
            event_log.log (current_tick,
                           agent->getId () + " OPEN " + op.getPath () + " -> odbijeno, nastao bi ciklus " + cycle);
            rejected_locks.push_back ("[" + std::to_string (current_tick) + "] " + agent->getId () +
                                      " nije dobio zakljucavanje nad " + op.getPath () + " zbog ciklusa " + cycle);
            agent->advance_op ();
            if (!agent->has_next_op ()) mark_done (agent, current_tick);
        } else {
            agent->setState (AgentState::BLOCKED);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> blokiran, ceka " + holder);
        }
        return;
    }

    event_log.log (current_tick,
                   agent->getId () + " OPEN " + op.getPath () + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void OperationExecutor::handle_close (std::shared_ptr<Agent> agent, int current_tick,
                                      std::function<void (const std::string&)> unblock_cb) {
    const Operation& op = agent->current_op ();
    VFSResult res = vfs.close (agent->getId (), op.getHandle ());
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " CLOSE " + op.getHandle ());
        deadlock_graph.remove_edges_for (agent->getId ());
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent, current_tick);
        return;
    }
    event_log.log (current_tick,
                   agent->getId () + " CLOSE " + op.getHandle () + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}
