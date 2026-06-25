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
        update_gantt ();

        // Stall detekcija: svi running agenti su na retry_cooldown -> nema napretka
        auto running = scheduler->get_running ();
        if (!running.empty ()) {
            bool all_stalled = true;
            for (const auto& a : running) {
                if (!a->getRetryCooldown ()) { all_stalled = false; break; }
            }
            if (all_stalled) {
                for (auto& a : running) {
                    a->setState (AgentState::DONE);
                    a->setEndTime (current_tick);
                    event_log.log (current_tick,
                        "Agent " + a->getId () + " zavrsio (nije mogao dobiti zakljucavanje)");
                }
            }
        }

        // Resetuj flagove za sljedeci tik
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

void Simulator::init () {
    vfs = std::make_unique<InMemoryVFS> ();

    for (const auto& mount : cfg.mounts) {
        VFSResult res = vfs->mount (mount);
        if (res != VFSResult::OK) {
            throw std::runtime_error ("Mount neuspjesan za source='" + mount.source
                                      + "' target='" + mount.target + "'");
        }
    }

    scheduler = std::make_unique<PriorityPreemptiveScheduler> (cfg.settings.max_running_agents);
    last_slot_agent.resize (cfg.settings.max_running_agents, "");

    for (const auto& agent_cfg : cfg.agents) {
        auto parsed = AgentParser::Parse (agent_cfg);
        auto agent  = std::make_shared<Agent> (std::move (parsed));
        all_agents.push_back (agent);
        scheduler->add_agent (agent);
        event_log.log (agent_cfg.arrival_time,
                       "Agent " + agent_cfg.id + " stigao, prioritet=" + std::to_string (agent_cfg.priority));
    }
}

void Simulator::step () {
    scheduler->tick (current_tick);

    for (auto& agent : scheduler->get_running ()) {
        if (!agent->has_next_op ()) {
            agent->setState (AgentState::DONE);
            agent->setEndTime (current_tick);
            event_log.log (current_tick, "Agent " + agent->getId () + " zavrsio");
            continue;
        }

        if (agent->getStartTime () == -1) {
            agent->setStartTime (current_tick);
        }

        // Preskoci agenta koji je preuzeo slot upravo ovog tika (preemption)
        // Agenti koji su slobodnim ulaskom dobili slot se NE preskacaju
        if (agent->getJustPreempted ()) continue;

        // Preskoci agenta ciji je OPEN odbijen zbog ciklusa (retry sljedeci tik)
        if (agent->getRetryCooldown ()) continue;

        execute_operation (agent);
    }

    try_unblock_agents ();
}

void Simulator::execute_operation (std::shared_ptr<Agent> agent) {
    const Operation& op = agent->current_op ();

    switch (op.getType ()) {
        case OperationType::THINK: {
            if (agent->getThinkTicksRemaining () == 0) {
                agent->startThink (op.getThinkingDuration ());
                event_log.log (current_tick,
                               agent->getId () + " THINK " + std::to_string (op.getThinkingDuration ()));
            }
            agent->tickThink ();
            if (agent->getThinkTicksRemaining () == 0) {
                agent->advance_op ();
            }
            break;
        }
        case OperationType::READ: {
            std::string content;
            VFSResult res = vfs->read (agent->getId (), op.getHandle (), content);
            if (res == VFSResult::OK) {
                event_log.log (current_tick,
                               agent->getId () + " READ " + op.getHandle () + " -> \"" + content + "\"");
                agent->advance_op ();
            } else {
                event_log.log (current_tick, agent->getId () + " READ "
                               + op.getHandle () + " -> greska");
            }
            break;
        }
        case OperationType::WRITE: {
            VFSResult res = vfs->write (agent->getId (), op.getHandle (), op.getData ());
            if (res == VFSResult::OK) {
                event_log.log (current_tick, agent->getId () + " WRITE " + op.getHandle ());
                agent->advance_op ();
            }
            break;
        }
        case OperationType::APPEND: {
            VFSResult res = vfs->append (agent->getId (), op.getHandle (), op.getData ());
            if (res == VFSResult::OK) {
                event_log.log (current_tick, agent->getId () + " APPEND "
                               + op.getHandle () + " \"" + op.getData () + "\"");
                agent->advance_op ();
            }
            break;
        }
        case OperationType::OPEN:  { handle_open  (agent, op); break; }
        case OperationType::CLOSE: { handle_close (agent, op); break; }
    }
}

void Simulator::handle_open (std::shared_ptr<Agent> agent, const Operation& op) {
    VFSResult res = vfs->open (agent->getId (), op.getPath (), op.getMode (), op.getHandle ());

    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath ()
                       + " " + op.getMode () + " as " + op.getHandle () + " -> zakljucano");
        agent->advance_op ();
        return;
    }

    if (res == VFSResult::WOULD_BLOCK) {
        std::string holder = vfs->get_lock_holder (op.getPath ());
        if (!holder.empty () && deadlock_graph.would_create_cycle (agent->getId (), holder)) {
            std::string cycle = deadlock_graph.get_cycle_path (agent->getId (), holder);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath ()
                           + " -> odbijeno, nastao bi ciklus " + cycle);
            rejected_locks.push_back ("[" + std::to_string (current_tick) + "] "
                                      + agent->getId () + " nije dobio zakljucavanje nad "
                                      + op.getPath () + " zbog ciklusa " + cycle);
            agent->setRetryCooldown (true);
        } else {
            if (!holder.empty ()) deadlock_graph.add_edge (agent->getId (), holder);
            agent->setState (AgentState::BLOCKED);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath ()
                           + " -> blokiran, ceka " + holder);
        }
        return;
    }

    event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath ()
                   + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void Simulator::handle_close (std::shared_ptr<Agent> agent, const Operation& op) {
    VFSResult res = vfs->close (agent->getId (), op.getHandle ());
    if (res == VFSResult::OK) {
        event_log.log (current_tick, agent->getId () + " CLOSE " + op.getHandle ());
        deadlock_graph.remove_edges_for (agent->getId ());
        agent->advance_op ();
        try_unblock_agents ();
        return;
    }
    event_log.log (current_tick, agent->getId () + " CLOSE " + op.getHandle ()
                   + " -> greska " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void Simulator::try_unblock_agents () {
    for (auto& agent : all_agents) {
        if (agent->getState () != AgentState::BLOCKED) continue;
        const Operation& op = agent->current_op ();
        if (op.getType () != OperationType::OPEN) continue;
        std::string holder = vfs->get_lock_holder (op.getPath ());
        if (!holder.empty ()) continue;
        deadlock_graph.remove_edges_for (agent->getId ());
        scheduler->unblock_agent (agent->getId ());
        event_log.log (current_tick, agent->getId () + " deblokiran, ponavlja OPEN " + op.getPath ());
    }
}

void Simulator::update_gantt () {
    auto running = scheduler->get_running ();
    std::vector<std::string> current_slot_agent (cfg.settings.max_running_agents, "");
    int idx = 0;
    for (const auto& agent : running) {
        if (idx < cfg.settings.max_running_agents)
            current_slot_agent[idx++] = agent->getId ();
    }

    for (int slot = 0; slot < cfg.settings.max_running_agents; slot++) {
        const std::string& agent_now  = current_slot_agent[slot];
        const std::string& agent_prev = last_slot_agent[slot];
        if (agent_now != agent_prev) {
            if (!gantt[slot].empty ())
                gantt[slot].back ().end = current_tick;
            gantt[slot].push_back ({current_tick, current_tick + 1, agent_now});
            last_slot_agent[slot] = agent_now;
        } else {
            if (gantt[slot].empty ())
                gantt[slot].push_back ({current_tick, current_tick + 1, agent_now});
            else
                gantt[slot].back ().end = current_tick + 1;
        }
    }
}

void Simulator::update_wait_times () {
    std::unordered_set<std::string> running_ids;
    for (const auto& agent : scheduler->get_running ())
        running_ids.insert (agent->getId ());

    for (auto& agent : all_agents) {
        if (agent->getState () == AgentState::READY
            && agent->getArrivalTime () <= current_tick
            && !running_ids.count (agent->getId ()))
            agent->addWaitTime (1);
    }
    for (auto& agent : all_agents) {
        if (agent->getState () == AgentState::BLOCKED)
            agent->incrementBlockedTime ();
    }
}

void Simulator::print_gantt (std::ostream& out) const {
    out << "\n=== Gantova karta ===\n";
    for (const auto& [slot, segments] : gantt) {
        out << "slot_" << (slot + 1) << ": ";
        bool first = true;
        for (const auto& seg : segments) {
            if (!first) out << " | ";
            out << "[" << seg.start << "," << seg.end << "] "
                << (seg.agent_id.empty () ? "idle" : seg.agent_id);
            first = false;
        }
        out << "\n";
    }
}

void Simulator::print_agent_summary (std::ostream& out) const {
    out << "\n=== Zavrsno stanje agenata ===\n";
    out << std::left
        << std::setw (10) << "Agent"
        << std::setw (14) << "Status"
        << std::setw (10) << "Dolazak"
        << std::setw (10) << "Pocetak"
        << std::setw (10) << "Kraj"
        << std::setw (12) << "Cekanje"
        << std::setw (12) << "Blokiran"
        << "Preuzimanja\n";
    for (const auto& agent : all_agents) {
        std::string status;
        switch (agent->getState ()) {
            case AgentState::DONE:    status = "zavrsen";     break;
            case AgentState::BLOCKED: status = "blokiran";   break;
            case AgentState::READY:   status = "spreman";    break;
            case AgentState::RUNNING: status = "pokrenut";   break;
            case AgentState::STOPPED: status = "zaustavljen"; break;
        }
        out << std::left
            << std::setw (10) << agent->getId ()
            << std::setw (14) << status
            << std::setw (10) << agent->getArrivalTime ()
            << std::setw (10) << (agent->getStartTime () == -1 ? 0 : agent->getStartTime ())
            << std::setw (10) << (agent->getEndTime ()   == -1 ? 0 : agent->getEndTime ())
            << std::setw (12) << agent->getWaitTime ()
            << std::setw (12) << agent->getBlockedTime ()
            << agent->getPreemptions () << "\n";
    }
}

void Simulator::print_rejected_locks (std::ostream& out) const {
    out << "\n=== Odbijena zakljucavanja ===\n";
    if (rejected_locks.empty ()) {
        out << "Nema odbijenih zakljucavanja.\n";
        return;
    }
    for (const auto& msg : rejected_locks)
        out << msg << "\n";
}

void Simulator::print_statistics (std::ostream& out) const {
    out << "\n=== Statistika ===\n";
    out << "Broj sprijecenih zastoja: " << rejected_locks.size () << "\n";
    double total_wait = 0, total_blocked = 0;
    int count = (int)all_agents.size ();
    for (const auto& a : all_agents) {
        total_wait    += a->getWaitTime ();
        total_blocked += a->getBlockedTime ();
    }
    if (count > 0) {
        out << std::fixed << std::setprecision (2);
        out << "Prosjecno vrijeme cekanja: "   << total_wait    / count << "\n";
        out << "Prosjecno vrijeme blokiranja: " << total_blocked / count << "\n";
    }
}
