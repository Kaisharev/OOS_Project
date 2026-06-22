#include "InMemoryVFS.hpp"

std::string InMemoryVFS::read_content (const std::filesystem::path& file_path) {
    std::ifstream file (file_path);
    if (!file.is_open ()) {
        std::cerr << "Ne mogu otvoriti fajl: " << file_path << std::endl;
        return "";
    }
    std::string content{std::istreambuf_iterator<char>{file}, {}};
    file.close ();
    return content;
}
VFSResult InMemoryVFS::mount (const MountPoint& mount_point) {
    std::filesystem::path mount_path = mount_point.source;
    if (!std::filesystem::exists (mount_path)) {
        std::cerr << "Mount path ne postoji: " << mount_path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    if (!std::filesystem::is_directory (mount_path)) {
        std::cerr << "Mount path nije direktorij: " << mount_path << std::endl;
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
// TODO:Bacaj изузетке ko pokemone. - Реко Јоле мора на српском!
// od kud sake na nebesima?
VFSResult InMemoryVFS::open (const std::string& agent_id, const std::string& path, const std::string& mode,
                             const std::string& handle) {
    const auto node = resolve_path (path);
    if (node == std::nullopt) {
        std::cerr << "Fajl nije pronađen: " << path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    if (mode != "r" && mode != "w" && mode != "rw") {
        std::cerr << "Neispravan mod: " << mode << std::endl;
        return VFSResult::INVALID_MODE;
    }
    if (node->type == VFSNodeType::DIRECTORY) {
        std::cerr << "Ne mogu otvoriti direktorij kao fajl: " << path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    if (node->is_read_only && (mode == "w" || mode == "rw")) {
        std::cerr << "Pristup nije dozvoljen: " << path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }
    if (agent_handles[agent_id].find (handle) != agent_handles[agent_id].end ()) {
        std::cerr << "Handle već postoji: " << handle << std::endl;
        return VFSResult::INVALID_HANDLE;
    }
    bool locked = false;

    if (mode == "r") {
        locked = locks[path].try_lock_read (agent_id);
    } else {
        locked = locks[path].try_lock_write (agent_id);
    }
    if (!locked) {
        std::cerr << "Ne mogu zauzeti fajl: " << path << std::endl;
        return VFSResult::WOULD_BLOCK;
    }
    agent_handles[agent_id][handle] = FileHandle{path, mode};
    return VFSResult::OK;
}
VFSResult InMemoryVFS::read (const std::string& agent_id, const std::string& handle, std::string& out_content) {
    if (agent_handles[agent_id].find (handle) == agent_handles[agent_id].end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return VFSResult::INVALID_HANDLE;
    }
    FileHandle& file_handle = agent_handles[agent_id][handle];
    if (file_handle.mode == "w") {
        std::cerr << "Nije dozvoljeno čitanje iz fajla otvorenog u write modu: " << file_handle.vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }
    if (resolve_path (file_handle.vfs_path) == std::nullopt) {
        std::cerr << "Fajl nije pronađen: " << file_handle.vfs_path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    out_content = nodes[file_handle.vfs_path].file_content;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::write (const std::string& agent_id, const std::string& handle, const std::string& data) {
    if (agent_handles[agent_id].find (handle) == agent_handles[agent_id].end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return VFSResult::INVALID_HANDLE;
    }
    FileHandle& file_handle = agent_handles[agent_id][handle];
    if (file_handle.mode == "r") {
        std::cerr << "Nije dozvoljeno pisanje u fajl otvoren za čitanje: " << file_handle.vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }
    if (resolve_path (file_handle.vfs_path) == std::nullopt) {
        std::cerr << "Fajl nije pronađen: " << file_handle.vfs_path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    nodes[file_handle.vfs_path].file_content = data;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::append (const std::string& agent_id, const std::string& handle, const std::string& data) {
    if (agent_handles[agent_id].find (handle) == agent_handles[agent_id].end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return VFSResult::INVALID_HANDLE;
    }
    FileHandle& file_handle = agent_handles[agent_id][handle];
    if (file_handle.mode == "r") {
        std::cerr << "Nije dozvoljeno dodavanje u fajl otvoren za čitanje: " << file_handle.vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }
    if (resolve_path (file_handle.vfs_path) == std::nullopt) {
        std::cerr << "Fajl nije pronađen: " << file_handle.vfs_path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    nodes[file_handle.vfs_path].file_content += data;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::close (const std::string& agent_id, const std::string& handle) {
    if (agent_handles[agent_id].find (handle) == agent_handles[agent_id].end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return VFSResult::INVALID_HANDLE;
    }
    FileHandle file_handle = agent_handles[agent_id][handle];
    if (file_handle.mode == "r") {
        if (!locks[file_handle.vfs_path].unlock_read (agent_id)) {
            std::cerr << "Nije dozvoljen pristup fajlu: " << file_handle.vfs_path << std::endl;
            return VFSResult::PERMISSION_DENIED;
        }
    } else {
        if (!locks[file_handle.vfs_path].unlock_write (agent_id)) {
            std::cerr << "Nije dozvoljen pristup fajlu: " << file_handle.vfs_path << std::endl;
            return VFSResult::PERMISSION_DENIED;
        }
    }
    agent_handles[agent_id].erase (handle);
    return VFSResult::OK;
}

void InMemoryVFS::dump (std::ostream& out) const {
    for (const auto& [path, node] : nodes) {
        out << path << " (" << (node.type == VFSNodeType::FILE ? "FILE" : "DIRECTORY") << ") "
            << "[" << (node.is_read_only ? "RO" : "RW") << "]\n";
    }
}