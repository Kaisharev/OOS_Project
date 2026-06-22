#pragma once
#include <ostream>
#include <string>

#include "VFSResult.hpp"
#include "core/MountPoint.hpp"

class IVFS {
    public:
        virtual ~IVFS () = default;

        virtual VFSResult mount (const MountPoint& mount_point) = 0;

        virtual VFSResult open (const std::string& agent_id, const std::string& path, const std::string& mode,
                                const std::string& handle) = 0;

        virtual VFSResult read (const std::string& agent_id, const std::string& handle, std::string& out_content) = 0;

        virtual VFSResult write (const std::string& agent_id, const std::string& handle, const std::string& data) = 0;

        virtual VFSResult append (const std::string& agent_id, const std::string& handle, const std::string& data) = 0;

        virtual VFSResult close (const std::string& agent_id, const std::string& handle) = 0;

        virtual void dump (std::ostream& out) const = 0;

        // Return id of agent holding lock for given VFS path, or empty if none
        virtual std::string get_lock_holder (const std::string& path) const = 0;
};