#include "EventLog.hpp"

void EventLog::log (int time, const std::string& message) {
    events.push_back ({time, message});
}

void EventLog::print (std::ostream& out) const {
    out << "=== Dnevnik događaja ===\n";
    for (const auto& event : events) {
        out << "[" << event.time << "] " << event.message << "\n";
    }
}

const std::vector<Event>& EventLog::get_events () const {
    return events;
}