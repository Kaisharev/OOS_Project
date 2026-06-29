#include "Simulator.hpp"

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
    gantt_tracker->print (std::cout);
    reporter.print_agent_summary (std::cout, all_agents);
    reporter.print_rejected_locks (std::cout, rejected_locks);
    vfs->dump (std::cout);
    reporter.print_statistics (std::cout, all_agents, rejected_locks);
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
    gantt_tracker = std::make_unique<GanttTracker> (cfg.settings.max_running_agents);

    executor = std::make_unique<OperationExecutor> (*vfs, event_log, deadlock_graph, rejected_locks, [this] (const std::string& id) {
        for (auto& a : all_agents)
            if (a->getId () == id) {
                mark_done (a);
                return;
            }
    });

    for (const auto& agent_cfg : cfg.agents) {
        auto parsed = AgentParser::Parse (agent_cfg);
        auto agent = std::make_shared<Agent> (std::move (parsed));
        all_agents.push_back (agent);
        scheduler->add_agent (agent);
        event_log.log (agent_cfg.arrival_time, "Agent " + agent_cfg.id + " stigao, prioritet=" + std::to_string (agent_cfg.priority));
    }

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
    gantt_tracker->update (0, slot_agents);
    current_tick = saved_tick;
}

void Simulator::step () {
    scheduler->tick (current_tick);

    for (auto& agent : scheduler->get_running ()) {
        if (agent->getState () == AgentState::DONE) continue;
        if (agent->getStartTime () == -1) agent->setStartTime (current_tick);
        if (agent->getJustPreempted ()) continue;
        if (agent->getRetryCooldown ()) continue;
        executor->execute (agent, current_tick);
    }

    try_unblock_agents ();
    gantt_tracker->update (current_tick, scheduler->get_slot_agents ());
}

void Simulator::try_unblock_agents () {
    executor->try_unblock_agents (
        all_agents,
        [this] (const std::string& id) {
            scheduler->unblock_agent (id);
        },
        current_tick);
}

void Simulator::mark_done (std::shared_ptr<Agent> agent) {
    gantt_tracker->close_agent (agent->getId (), current_tick);
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
