#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../core/FileHandle.hpp"
#include "IVFS.hpp"
#include "RWLock.hpp"
#include "VFSNode.hpp"

class InMemoryVFS : public IVFS {
    public:
        InMemoryVFS () = default;

        VFSResult mount (const MountPoint& mount_point) override;
        VFSResult open (const std::string& agent_id, const std::string& path, const std::string& mode,
                        const std::string& handle) override;
        VFSResult read (const std::string& agent_id, const std::string& handle, std::string& out_content) override;
        VFSResult write (const std::string& agent_id, const std::string& handle, const std::string& data) override;
        VFSResult append (const std::string& agent_id, const std::string& handle, const std::string& data) override;
        VFSResult close (const std::string& agent_id, const std::string& handle) override;
        void dump (std::ostream& out) const override;
        std::string get_lock_holder (const std::string& path) const override;

        std::string read_content (const std::filesystem::path& file_path);

    private:
        std::unordered_map<std::string, VFSNode> nodes;
        std::unordered_map<std::string, std::unordered_map<std::string, FileHandle>> agent_handles;
        std::unordered_map<std::string, RWLock> locks;

        VFSNode* resolve_path (const std::string& file_path);
        VFSNode* resolve_file_node (const std::string& file_path);
        FileHandle* resolve_handle (const std::string& agent_id, const std::string& handle);
};
