#pragma once
#include <ostream>
#include <string>
#include <vector>

struct Event {
        int time;
        std::string message;
};

class EventLog {
    public:
        void log (int time, const std::string& message);
        void print (std::ostream& out) const;
        const std::vector<Event>& get_events () const;

    private:
        std::vector<Event> events;
};