#include "GanttTracker.hpp"

#include <iostream>

GanttTracker::GanttTracker (int num_slots) : num_slots (num_slots) {
    last_slot_agent.resize (num_slots, "");
}

void GanttTracker::update (int current_tick, const std::vector<std::shared_ptr<Agent>>& slot_agents) {
    for (int slot = 0; slot < (int)slot_agents.size (); slot++) {
        std::string agent_now =
            (slot_agents[slot] && slot_agents[slot]->getState () == AgentState::RUNNING) ? slot_agents[slot]->getId () : "";
        const std::string& agent_prev = last_slot_agent[slot];
        if (agent_now != agent_prev) {
            if (!gantt[slot].empty ()) gantt[slot].back ().end = current_tick;
            gantt[slot].push_back ({current_tick, current_tick + 1, agent_now});
            last_slot_agent[slot] = agent_now;
        } else {
            if (gantt[slot].empty ()) {
                gantt[slot].push_back ({current_tick, current_tick + 1, agent_now});
            } else {
                auto& last = gantt[slot].back ();
                if (!(last.agent_id.empty () && last.start == last.end)) {
                    last.end = current_tick + 1;
                }
            }
        }
    }
}

void GanttTracker::close_agent (const std::string& agent_id, int current_tick) {
    for (int slot = 0; slot < num_slots; slot++) {
        if (last_slot_agent[slot] == agent_id) {
            if (!gantt[slot].empty ()) gantt[slot].back ().end = current_tick;
            last_slot_agent[slot] = "";
            gantt[slot].push_back ({current_tick, current_tick, ""});
        }
    }
}

void GanttTracker::print (std::ostream& out) const {
    out << "\n=== Gantova karta ===\n";
    for (const auto& [slot, segments] : gantt) {
        out << "slot_" << (slot + 1) << ": ";
        bool first = true;
        for (const auto& seg : segments) {
            if (seg.start == seg.end) continue;
            if (!first) out << " | ";
            out << "[" << seg.start << "," << seg.end << "] " << (seg.agent_id.empty () ? "idle" : seg.agent_id);
            first = false;
        }
        out << "\n";
    }
}
