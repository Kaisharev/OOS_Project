#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../core/FileHandle.hpp"
#include "IVFS.hpp"
#include "RWLock.hpp"
#include "VFSNode.hpp"

class InMemoryVFS : public IVFS {
    public:
        InMemoryVFS () = default;
        VFSResult open (const std::string& agent_id, const std::string& path, const std::string& mode, const std::string& handle);
        VFSResult mount (const MountPoint& mount_point);

        VFSResult read (const std::string& agent_id, const std::string& handle, std::string& out_content);

        VFSResult write (const std::string& agent_id, const std::string& handle, const std::string& data);

        VFSResult append (const std::string& agent_id, const std::string& handle, const std::string& data);

        VFSResult close (const std::string& agent_id, const std::string& handle);
        std::string read_content (const std::filesystem::path& file_path);
        void dump (std::ostream& out) const;

    private:
        std::unordered_map<std::string, VFSNode> nodes;
        std::optional<VFSNode> resolve_path (const std::string& file_path);
        std::unordered_map<std::string, std::unordered_map<std::string, FileHandle>> agent_handles;
        std::unordered_map<std::string, RWLock> locks;
};