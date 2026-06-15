#pragma once
#include <set>
#include <string>

class RWLock {
    private:
        std::set<std::string> readers;
        std::string writer;

    public:
        bool try_lock_read (const std::string& agent_id);
        bool try_lock_write (const std::string& agent_id);
        bool unlock_read (const std::string& agent_id);
        bool unlock_write (const std::string& agent_id);
        std::string get_holder () const;
};