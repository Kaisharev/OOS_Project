#include "Simulator.hpp"

#include <iomanip>
#include <iostream>

#include "../agent/Parser/AgentParser.hpp"
#include "../scheduler/PriorityPreemptiveScheduler.hpp"

Simulator::Simulator (const Config& cfg) : cfg (cfg) {}

void Simulator::run () {
    init ();
    while (!scheduler->all_done ()) {
        step ();
        update_wait_times ();

        if (is_global_deadlock ()) {
            event_log.log (current_tick, "[DEADLOCK] Globalni zastoj detektovan - simulacija prekinuta");
            for (auto& a : all_agents) {
                if (a->getState () == AgentState::BLOCKED || a->getState () == AgentState::RUNNING ||
                    a->getState () == AgentState::READY) {
                    a->setState (AgentState::STOPPED);
                    if (a->getEndTime () == -1) a->setEndTime (current_tick);
                }
            }
            break;
        }

        for (auto& a : all_agents) {
            a->setRetryCooldown (false);
            a->setJustPreempted (false);
        }
        current_tick++;
    }
    event_log.print (std::cout);
    print_gantt (std::cout);
    print_agent_summary (std::cout);
    print_rejected_locks (std::cout);
    vfs->dump (std::cout);
    print_statistics (std::cout);
}

bool Simulator::is_global_deadlock () const {
    bool any_blocked = false;
    for (const auto& a : all_agents) {
        const auto state = a->getState ();
        if (state == AgentState::DONE || state == AgentState::STOPPED) continue;
        if (state == AgentState::RUNNING) return false;
        if (state == AgentState::READY) return false;
        if (state == AgentState::BLOCKED) any_blocked = true;
        if (a->getArrivalTime () > current_tick) return false;
    }
    return any_blocked;
}

void Simulator::init () {
    vfs = std::make_unique<InMemoryVFS> ();
    for (const auto& mount : cfg.mounts) {
        VFSResult res = vfs->mount (mount);
        if (res != VFSResult::OK)
            throw std::runtime_error ("Mount neuspjesan za source='" + mount.source + "' target='" + mount.target + "'");
    }
    scheduler = std::make_unique<PriorityPreemptiveScheduler> (cfg.settings.max_running_agents);
    last_slot_agent.resize (cfg.settings.max_running_agents, "");
    for (const auto& agent_cfg : cfg.agents) {
        auto parsed = AgentParser::Parse (agent_cfg);
        auto agent = std::make_shared<Agent> (std::move (parsed));
        all_agents.push_back (agent);
        scheduler->add_agent (agent);
        event_log.log (agent_cfg.arrival_time, "Agent " + agent_cfg.id + " stigao, prioritet=" + std::to_string (agent_cfg.priority));
    }

    // Tick 0: popuni slotove i zabilježi Gantovu kartu od tika 0
    int saved_tick = current_tick;
    current_tick = 0;
    scheduler->tick (0);
    auto slot_agents = scheduler->get_slot_agents ();
    for (int i = 0; i < (int)slot_agents.size (); i++) {
        if (slot_agents[i]) {
            event_log.log (0, "slot_" + std::to_string (i + 1) + " <- " + slot_agents[i]->getId ());
            slot_agents[i]->setStartTime (0);
        }
    }
    update_gantt ();
    current_tick = saved_tick;
}

void Simulator::step () {
    scheduler->tick (current_tick);

    for (auto& agent : scheduler->get_running ()) {
        if (agent->getState () == AgentState::DONE) continue;
        if (agent->getStartTime () == -1) agent->setStartTime (current_tick);
        if (agent->getJustPreempted ()) continue;
        if (agent->getRetryCooldown ()) continue;
        execute_operation (agent);
    }

    try_unblock_agents ();

    // Gantt se uzima NAKON svih promjena u tiku (deblokiranje, završavanje)
    update_gantt ();
}

void Simulator::execute_agent_tick (std::shared_ptr<Agent> agent) {
    execute_operation (agent);
}

void Simulator::execute_operation (std::shared_ptr<Agent> agent) {
    if (!agent->has_next_op ()) return;
    const Operation& op = agent->current_op ();
    switch (op.getType ()) {
        case OperationType::THINK: {
            if (agent->getThinkTicksRemaining () == 0) {
                agent->startThink (op.getThinkingDuration ());
                event_log.log (current_tick, agent->getId () + " THINK " + std::to_string (op.getThinkingDuration ()));
            }
            agent->tickThink ();
            if (agent->getThinkTicksRemaining () == 0) {
                agent->advance_op ();
                if (!agent->has_next_op ()) mark_done (agent);
            }
            break;
        }
        case OperationType::READ: {
            std::string content;
            VFSResult res = vfs->read (agent->getId (), op.getHandle (), content);
            if (res == VFSResult::OK) {
                event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> \"" + content + "\"");
                agent->advance_op ();
                if (!agent->has_next_op ()) mark_done (agent);
            } else {
                event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> greska");
            }
            break;
        }
        case OperationType::WRITE: {
            VFSResult res = vfs->write (agent->getId (), op.getHandle (), op.getData ());
            if (res == VFSResult::OK) {
                event_log.log (current_tick, agent->getId () + " WRITE " + op.getHandle ());
                agent->advance_op ();
                if (!agent->has_next_op ()) mark_done (agent);
            }
            break;
        }
        case OperationType::APPEND: {
            VFSResult res = vfs->append (agent->getId (), op.getHandle (), op.getData ());
            if (res == VFSResult::OK) {
                event_log.log (current_tick, agent->getId () + " APPEND " + op.getHandle () + " \"" + op.getData () + "\"");
                agent->advance_op ();
                if (!agent->has_next_op ()) mark_done (agent);
            }
            break;
        }
        case OperationType::OPEN: {
            handle_open (agent, op);
            break;
        }
        case OperationType::CLOSE: {
            handle_close (agent, op);
            break;
        }
    }
}

void Simulator::handle_open (std::shared_ptr<Agent> agent, const Operation& op) {
    VFSResult res = vfs->open (agent->getId (), op.getPath (), op.getMode (), op.getHandle ());

    if (res == VFSResult::OK) {
        event_log.log (current_tick,
                       agent->getId () + " OPEN " + op.getPath () + " " + op.getMode () + " as " + op.getHandle () + " -> zakljucano");
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent);
        return;
    }

    if (res == VFSResult::WOULD_BLOCK) {
        std::string holder = vfs->get_lock_holder (op.getPath ());

        if (!holder.empty ()) deadlock_graph.add_edge (agent->getId (), holder);

        if (!holder.empty () && deadlock_graph.would_create_cycle_after_add (agent->getId ())) {
            deadlock_graph.remove_edges_for (agent->getId ());
            std::string cycle = deadlock_graph.get_cycle_path (agent->getId (), holder);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> odbijeno, nastao bi ciklus " + cycle);
            rejected_locks.push_back ("[" + std::to_string (current_tick) + "] " + agent->getId () + " nije dobio zakljucavanje nad " +
                                      op.getPath () + " zbog ciklusa " + cycle);
            agent->advance_op ();
            if (!agent->has_next_op ()) mark_done (agent);
        } else {
            agent->setState (AgentState::BLOCKED);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> blokiran, ceka " + holder);
        }
        return;
    }

    event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void Simulator::handle_close (std::shared_ptr<Agent> agent, const Operation& op) {
    VFSResult res = vfs->close (agent->getId (), op.getHandle ());
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " CLOSE " + op.getHandle ());
        deadlock_graph.remove_edges_for (agent->getId ());
        agent->advance_op ();
        if (!agent->has_next_op ()) mark_done (agent);
        try_unblock_agents ();
        return;
    }
    event_log.log (current_tick,
                   agent->getId () + " CLOSE " + op.getHandle () + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void Simulator::try_unblock_agents () {
    for (auto& agent : all_agents) {
        if (agent->getState () != AgentState::BLOCKED) continue;
        if (!agent->has_next_op ()) continue;
        const Operation& op = agent->current_op ();
        if (op.getType () != OperationType::OPEN) continue;
        std::string holder = vfs->get_lock_holder (op.getPath ());
        if (!holder.empty ()) continue;
        deadlock_graph.remove_edges_for (agent->getId ());
        scheduler->unblock_agent (agent->getId ());
        event_log.log (current_tick, agent->getId () + " deblokiran, ponavlja OPEN " + op.getPath ());
        execute_operation (agent);
    }
}

void Simulator::close_gantt_for_agent (const std::string& agent_id) {
    for (int slot = 0; slot < (int)last_slot_agent.size (); slot++) {
        if (last_slot_agent[slot] == agent_id) {
            if (!gantt[slot].empty ()) gantt[slot].back ().end = current_tick;
            last_slot_agent[slot] = "";
            gantt[slot].push_back ({current_tick, current_tick, ""});
        }
    }
}

void Simulator::mark_done (std::shared_ptr<Agent> agent) {
    agent->setState (AgentState::DONE);
    agent->setEndTime (current_tick);
    event_log.log (current_tick, "Agent " + agent->getId () + " zavrsio");
    close_gantt_for_agent (agent->getId ());
}

void Simulator::update_gantt () {
    auto slot_agents = scheduler->get_slot_agents ();
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

void Simulator::update_wait_times () {
    std::unordered_set<std::string> running_ids;
    for (const auto& agent : scheduler->get_running ()) running_ids.insert (agent->getId ());
    for (auto& agent : all_agents) {
        if (agent->getState () == AgentState::READY && agent->getArrivalTime () <= current_tick &&
            !running_ids.count (agent->getId ()))
            agent->addWaitTime (1);
    }
    for (auto& agent : all_agents)
        if (agent->getState () == AgentState::BLOCKED) agent->incrementBlockedTime ();
}

void Simulator::print_gantt (std::ostream& out) const {
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

void Simulator::print_agent_summary (std::ostream& out) const {
    out << "\n=== Zavrsno stanje agenata ===\n";
    out << std::left << std::setw (10) << "Agent" << std::setw (14) << "Status" << std::setw (10) << "Dolazak" << std::setw (10)
        << "Pocetak" << std::setw (10) << "Kraj" << std::setw (12) << "Cekanje" << std::setw (12) << "Blokiran"
        << "Preuzimanja\n";
    for (const auto& agent : all_agents) {
        std::string status;
        switch (agent->getState ()) {
            case AgentState::DONE:
                status = "zavrsen";
                break;
            case AgentState::BLOCKED:
                status = "blokiran";
                break;
            case AgentState::READY:
                status = "spreman";
                break;
            case AgentState::RUNNING:
                status = "pokrenut";
                break;
            case AgentState::STOPPED:
                status = "zaustavljen";
                break;
        }
        out << std::left << std::setw (10) << agent->getId () << std::setw (14) << status << std::setw (10) << agent->getArrivalTime ()
            << std::setw (10) << (agent->getStartTime () == -1 ? 0 : agent->getStartTime ()) << std::setw (10)
            << (agent->getEndTime () == -1 ? 0 : agent->getEndTime ()) << std::setw (12) << agent->getWaitTime () << std::setw (12)
            << agent->getBlockedTime () << agent->getPreemptions () << "\n";
    }
}

void Simulator::print_rejected_locks (std::ostream& out) const {
    out << "\n=== Odbijena zakljucavanja ===\n";
    if (rejected_locks.empty ()) {
        out << "Nema odbijenih zakljucavanja.\n";
        return;
    }
    for (const auto& msg : rejected_locks) out << msg << "\n";
}

void Simulator::print_statistics (std::ostream& out) const {
    out << "\n=== Statistika ===\n";
    out << "Broj sprijecenih zastoja: " << rejected_locks.size () << "\n";
    double total_wait = 0, total_blocked = 0;
    int count = (int)all_agents.size ();
    for (const auto& a : all_agents) {
        total_wait += a->getWaitTime ();
        total_blocked += a->getBlockedTime ();
    }
    if (count > 0) {
        out << std::fixed << std::setprecision (2);
        out << "Prosjecno vrijeme cekanja: " << total_wait / count << "\n";
        out << "Prosjecno vrijeme blokiranja: " << total_blocked / count << "\n";
    }
}
