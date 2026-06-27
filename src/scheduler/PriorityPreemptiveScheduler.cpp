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
        if (!slot) continue;
        const auto state = slot->getState ();
        if (state == AgentState::DONE || state == AgentState::STOPPED) {
            slot = nullptr;
        } else if (state == AgentState::BLOCKED) {
            blocked[slot->getId ()] = slot;
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
            auto incoming = ready_queue.top ();
            ready_queue.pop ();
            incoming->setState (AgentState::RUNNING);
            incoming->setJustPreempted (true);
            slot = incoming;
        }
    }
}

void PriorityPreemptiveScheduler::fill_slots () {
    std::unordered_set<std::string> already_in_slot;
    for (const auto& slot : running_slots)
        if (slot) already_in_slot.insert (slot->getId ());

    for (auto& slot : running_slots) {
        if (slot) continue;
        while (!ready_queue.empty () && already_in_slot.count (ready_queue.top ()->getId ())) ready_queue.pop ();
        if (ready_queue.empty ()) break;
        auto agent = ready_queue.top ();
        ready_queue.pop ();
        agent->setState (AgentState::RUNNING);
        slot = agent;
        already_in_slot.insert (agent->getId ());
    }
}

std::vector<std::shared_ptr<Agent>> PriorityPreemptiveScheduler::get_running () const {
    std::vector<std::shared_ptr<Agent>> running;
    for (const auto& slot : running_slots)
        if (slot && slot->getState () == AgentState::RUNNING) running.push_back (slot);
    return running;
}

std::vector<std::shared_ptr<Agent>> PriorityPreemptiveScheduler::get_slot_agents () const {
    return running_slots;
}

void PriorityPreemptiveScheduler::unblock_agent (const std::string& agent_id) {
    auto it = blocked.find (agent_id);
    if (it == blocked.end ()) return;
    it->second->setState (AgentState::READY);
    ready_queue.push (it->second);
    blocked.erase (it);
    // Odmah popuni slobodne slotove - agent ulazi u slot u istom tiku
    fill_slots ();
}

bool PriorityPreemptiveScheduler::all_done () const {
    if (!pending.empty ()) return false;
    if (!ready_queue.empty ()) return false;
    if (!blocked.empty ()) return false;
    for (const auto& slot : running_slots)
        if (slot && slot->getState () != AgentState::DONE && slot->getState () != AgentState::STOPPED) return false;
    return true;
}
