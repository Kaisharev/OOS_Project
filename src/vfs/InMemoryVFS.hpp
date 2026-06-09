#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include "IVFS.h"
#include "VFSNode.hpp"


class InMemoryVFS : public IVFS {
    public:
        InMemoryVFS () = default;
        ~InMemoryVFS () = default;
        VFSResult open (const std::string& agent_id, const std::string& path, const std::string& mode, const std::string& handle) = 0;
        VFSResult mount (const MountPoint& mount_point) = 0;

    private:
        std::unordered_map<std::string, VFSNode> nodes;
        std::string read_content (const std::filesystem::path& file_path);
        std::optional<VFSNode> resolve_path (const std::string& file_path);
};