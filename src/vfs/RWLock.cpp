#include "RWLock.hpp"

bool RWLock::try_lock_read (const std::string& agent_id) {
    if (!writer.empty () && writer != agent_id) {
        return false;
    }
    readers.insert (agent_id);
    return true;
}

bool RWLock::try_lock_write (const std::string& agent_id) {
    if (!writer.empty () && writer != agent_id) {
        return false;
    }
    if (!readers.empty () && (readers.size () > 1 || readers.count (agent_id) == 0)) {
        return false;
    }
    writer = agent_id;
    return true;
}

bool RWLock::unlock_read (const std::string& agent_id) {
    return readers.erase (agent_id) > 0;
}

bool RWLock::unlock_write (const std::string& agent_id) {
    if (writer == agent_id) {
        writer.clear ();
        return true;
    }
    return false;
}

std::string RWLock::get_holder () const {
    if (!writer.empty ()) {
        return writer;
    }
    if (!readers.empty ()) {
        return *readers.begin ();
    }
    return "";
}