#include "Simulator.hpp"

#include "../agent/Parser/AgentParser.hpp"
#include "../scheduler/PriorityPreemptiveScheduler.hpp"

Simulator::Simulator (const Config& cfg) : cfg (cfg) {}

void Simulator::run () {
    init ();
    while (!scheduler->all_done ()) {
        step ();
        current_tick++;
    }
    event_log.print (std::cout);
}

void Simulator::init () {
    vfs = std::make_unique<InMemoryVFS> ();

    for (const auto& mount : cfg.mounts) {
        VFSResult res = vfs->mount (mount);
        if (res != VFSResult::OK) {
            throw std::runtime_error ("Mount neuspješan: " + mount.source);
        }
    }

    scheduler = std::make_unique<PriorityPreemptiveScheduler> (cfg.settings.max_running_agents);

    for (const auto& agent_cfg : cfg.agents) {
        auto parsed = AgentParser::Parse (agent_cfg);
        auto agent = std::make_shared<Agent> (std::move (parsed));
        all_agents.push_back (agent);
        scheduler->add_agent (agent);
        event_log.log (agent_cfg.arrival_time, "Agent " + agent_cfg.id + " stigao, prioritet=" + std::to_string (agent_cfg.priority));
    }
}

void Simulator::step () {
    scheduler->tick (current_tick);

    for (auto& agent : scheduler->get_running ()) {
        if (!agent->has_next_op ()) {
            agent->setState (AgentState::DONE);
            agent->setEndTime (current_tick);
            event_log.log (current_tick, "Agent " + agent->getId () + " završio");
            continue;
        }

        if (agent->getStartTime () == -1) {
            agent->setStartTime (current_tick);
        }

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
                event_log.log (current_tick, agent->getId () + " THINK " + std::to_string (op.getThinkingDuration ()));
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
                event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> \"" + content + "\"");
                agent->advance_op ();
            } else {
                event_log.log (current_tick, agent->getId () + " READ " + op.getHandle () + " -> greška");
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
                event_log.log (current_tick, agent->getId () + " APPEND " + op.getHandle () + " \"" + op.getData () + "\"");
                agent->advance_op ();
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
                       agent->getId () + " OPEN " + op.getPath () + " " + op.getMode () + " as " + op.getHandle () + " -> zaključano");
        agent->advance_op ();
        return;
    }

    if (res == VFSResult::WOULD_BLOCK) {
        std::string holder = vfs->get_lock_holder (op.getPath ());

        if (deadlock_graph.would_create_cycle (agent->getId (), holder)) {
            std::string cycle = deadlock_graph.get_cycle_path (agent->getId (), holder);
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> odbijeno, nastao bi ciklus " + cycle);
        } else {
            deadlock_graph.add_edge (agent->getId (), holder);
            agent->setState (AgentState::BLOCKED);
            agent->incrementBlockedTime ();
            event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> blokiran, čeka " + holder);
        }
        return;
    }

    event_log.log (current_tick, agent->getId () + " OPEN " + op.getPath () + " -> greška " + std::to_string (static_cast<int> (res)));
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

    event_log.log (current_tick,
                   agent->getId () + " CLOSE " + op.getHandle () + " -> greška " + std::to_string (static_cast<int> (res)));
    agent->advance_op ();
}

void Simulator::try_unblock_agents () {
    for (auto& agent : all_agents) {
        if (agent->getState () != AgentState::BLOCKED) continue;

        const Operation& op = agent->current_op ();
        if (op.getType () != OperationType::OPEN) continue;

        std::string holder = vfs->get_lock_holder (op.getPath ());
        if (!holder.empty ()) {
            agent->incrementBlockedTime ();
            continue;
        }

        deadlock_graph.remove_edges_for (agent->getId ());
        scheduler->unblock_agent (agent->getId ());
        event_log.log (current_tick, agent->getId () + " deblokiran, ponavlja OPEN " + op.getPath ());
    }
}