#include "SimulationReporter.hpp"

#include <iomanip>

void SimulationReporter::print_agent_summary (std::ostream& out, const std::vector<std::shared_ptr<Agent>>& agents) const {
    out << "\n=== Zavrsno stanje agenata ===\n";
    out << std::left << std::setw (10) << "Agent" << std::setw (14) << "Status" << std::setw (10) << "Dolazak" << std::setw (10)
        << "Pocetak" << std::setw (10) << "Kraj" << std::setw (12) << "Cekanje" << std::setw (12) << "Blokiran"
        << "Preuzimanja\n";
    for (const auto& agent : agents) {
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
        out << std::left << std::setw (10) << agent->getId () << std::setw (14) << status << std::setw (10)
            << agent->getArrivalTime () << std::setw (10) << (agent->getStartTime () == -1 ? 0 : agent->getStartTime ())
            << std::setw (10) << (agent->getEndTime () == -1 ? 0 : agent->getEndTime ()) << std::setw (12) << agent->getWaitTime ()
            << std::setw (12) << agent->getBlockedTime () << agent->getPreemptions () << "\n";
    }
}

void SimulationReporter::print_rejected_locks (std::ostream& out, const std::vector<std::string>& rejected_locks) const {
    out << "\n=== Odbijena zakljucavanja ===\n";
    if (rejected_locks.empty ()) {
        out << "Nema odbijenih zakljucavanja.\n";
        return;
    }
    for (const auto& msg : rejected_locks) out << msg << "\n";
}

void SimulationReporter::print_statistics (std::ostream& out, const std::vector<std::shared_ptr<Agent>>& agents,
                                           const std::vector<std::string>& rejected_locks) const {
    out << "\n=== Statistika ===\n";
    out << "Broj sprijecenih zastoja: " << rejected_locks.size () << "\n";
    double total_wait = 0, total_blocked = 0;
    int count = (int)agents.size ();
    for (const auto& a : agents) {
        total_wait += a->getWaitTime ();
        total_blocked += a->getBlockedTime ();
    }
    if (count > 0) {
        out << std::fixed << std::setprecision (2);
        out << "Prosjecno vrijeme cekanja: " << total_wait / count << "\n";
        out << "Prosjecno vrijeme blokiranja: " << total_blocked / count << "\n";
    }
}
