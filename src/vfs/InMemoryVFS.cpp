#include "InMemoryVFS.hpp"

#include <filesystem>
#include <fstream>
#include <optional>

std::string InMemoryVFS::read_content (const std::filesystem::path& file_path) {
    std::ifstream file (file_path);
    if (!file.is_open ()) {
        return "";
    }
    std::string content ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
    file.close ();
    return content;
}
VFSResult InMemoryVFS::mount (const MountPoint& mount_point) {
    std::filesystem::path mount_path = mount_point.source;
    if (!std::filesystem::exists (mount_path)) {
        return VFSResult::NOT_FOUND;
    }
    if (!std::filesystem::is_directory (mount_path)) {
        return VFSResult::NOT_FOUND;
    }

    nodes[mount_point.target] =
        VFSNode{mount_path.filename ().string (), VFSNodeType::DIRECTORY, "", mount_point.mode == MountPoint::Mode::RO};

    for (const auto& entry : std::filesystem::recursive_directory_iterator (mount_path)) {
        if (entry.is_regular_file ()) {
            std::string relative_path = std::filesystem::relative (entry.path (), mount_path).string ();
            std::string vfs_path = mount_point.target + "/" + relative_path;
            std::string content = read_content (entry.path ());
            nodes[vfs_path] = VFSNode{relative_path, VFSNodeType::FILE, content, mount_point.mode == MountPoint::Mode::RO};
        } else {
            std::string relative_path = std::filesystem::relative (entry.path (), mount_path).string ();
            std::string vfs_path = mount_point.target + "/" + relative_path;
            nodes[vfs_path] = VFSNode{relative_path, VFSNodeType::DIRECTORY, "", mount_point.mode == MountPoint::Mode::RO};
        }
    }
    return VFSResult::OK;
}

std::optional<VFSNode> InMemoryVFS::resolve_path (const std::string& path) {
    if (nodes.find (path) != nodes.end ()) {
        return nodes[path];
    } else {
        return std::nullopt;
    }
}