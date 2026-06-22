#include "PriorityPreemptiveScheduler.hpp"

PriorityPreemptiveScheduler::PriorityPreemptiveScheduler (int max_running_agents) : max_running_agents (max_running_agents) {
    running_slots.resize (max_running_agents, nullptr);
}

void PriorityPreemptiveScheduler::add_agent (std::shared_ptr<Agent> agent) {
    pending.push_back (agent);
}

void PriorityPreemptiveScheduler::tick (int current_time) {
    move_arrived_agents (current_time);
    release_done_slots ();
    check_preemption ();
    fill_slots ();
}

void PriorityPreemptiveScheduler::move_arrived_agents (int current_time) {
    auto it = pending.begin ();
    while (it != pending.end ()) {
        if ((*it)->getArrivalTime () <= current_time) {
            (*it)->setState (AgentState::READY);
            ready_queue.push (*it);
            it = pending.erase (it);
        } else {
            ++it;
        }
    }
}

void PriorityPreemptiveScheduler::release_done_slots () {
    for (auto& slot : running_slots) {
        if (slot && slot->getState () != AgentState::RUNNING) {
            if (slot->getState () == AgentState::BLOCKED) {
                blocked[slot->getId ()] = slot;
            }
            slot = nullptr;
        }
    }
}

void PriorityPreemptiveScheduler::check_preemption () {
    if (ready_queue.empty ()) return;

    for (auto& slot : running_slots) {
        if (!slot) continue;
        if (ready_queue.empty ()) break;

        const auto& candidate = ready_queue.top ();
        if (candidate->getPriority () < slot->getPriority ()) {
            slot->setState (AgentState::READY);
            slot->incrementPreemptions ();
            ready_queue.push (slot);
            slot = ready_queue.top ();
            slot->setState (AgentState::RUNNING);
            ready_queue.pop ();
        }
    }
}

void PriorityPreemptiveScheduler::fill_slots () {
    for (auto& slot : running_slots) {
        if (slot) continue;
        if (ready_queue.empty ()) break;

        slot = ready_queue.top ();
        slot->setState (AgentState::RUNNING);
        ready_queue.pop ();
    }
}

std::vector<std::shared_ptr<Agent>> PriorityPreemptiveScheduler::get_running () const {
    std::vector<std::shared_ptr<Agent>> running;
    for (const auto& slot : running_slots) {
        if (slot && slot->getState () == AgentState::RUNNING) {
            running.push_back (slot);
        }
    }
    return running;
}

void PriorityPreemptiveScheduler::unblock_agent (const std::string& agent_id) {
    auto it = blocked.find (agent_id);
    if (it == blocked.end ()) return;

    it->second->setState (AgentState::READY);
    ready_queue.push (it->second);
    blocked.erase (it);
}

bool PriorityPreemptiveScheduler::all_done () const {
    if (!pending.empty ()) return false;
    if (!ready_queue.empty ()) return false;
    if (!blocked.empty ()) return false;
    for (const auto& slot : running_slots) {
        if (slot) return false;
    }
    return true;
}