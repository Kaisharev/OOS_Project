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

VFSNode* InMemoryVFS::resolve_path (const std::string& path) {
    auto it = nodes.find (path);
    if (it != nodes.end ()) {
        return &it->second;
    }
    return nullptr;
}

VFSNode* InMemoryVFS::resolve_file_node (const std::string& file_path) {
    VFSNode* node = resolve_path (file_path);
    if (node == nullptr) {
        std::cerr << "Fajl nije pronađen: " << file_path << std::endl;
        return nullptr;
    }
    return node;
}

FileHandle* InMemoryVFS::resolve_handle (const std::string& agent_id, const std::string& handle) {
    auto agent_it = agent_handles.find (agent_id);
    if (agent_it == agent_handles.end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return nullptr;
    }

    auto handle_it = agent_it->second.find (handle);
    if (handle_it == agent_it->second.end ()) {
        std::cerr << "Neispravan handle: " << handle << std::endl;
        return nullptr;
    }

    return &handle_it->second;
}
// TODO:Bacaj изузетке ko pokemone. - Реко Јоле мора на српском!
// od kud sake na nebesima?
VFSResult InMemoryVFS::open (const std::string& agent_id, const std::string& path, const std::string& mode,
                             const std::string& handle) {
    VFSNode* node = resolve_path (path);
    if (node == nullptr) {
        std::cerr << "Fajl nije pronađen: " << path << std::endl;
        return VFSResult::NOT_FOUND;
    }
    if (mode != "r" && mode != "w" && mode != "rw") {
        std::cerr << "Neispravan mode: " << mode << std::endl;
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
    FileHandle* file_handle = resolve_handle (agent_id, handle);
    if (file_handle == nullptr) {
        return VFSResult::INVALID_HANDLE;
    }

    if (file_handle->mode == "w") {
        std::cerr << "Nije dozvoljeno čitanje iz fajla otvorenog u write modu: " << file_handle->vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }

    VFSNode* node = resolve_file_node (file_handle->vfs_path);
    if (node == nullptr) {
        return VFSResult::NOT_FOUND;
    }

    out_content = node->file_content;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::write (const std::string& agent_id, const std::string& handle, const std::string& data) {
    FileHandle* file_handle = resolve_handle (agent_id, handle);
    if (file_handle == nullptr) {
        return VFSResult::INVALID_HANDLE;
    }

    if (file_handle->mode == "r") {
        std::cerr << "Nije dozvoljeno pisanje u fajl otvoren za čitanje: " << file_handle->vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }

    VFSNode* node = resolve_file_node (file_handle->vfs_path);
    if (node == nullptr) {
        return VFSResult::NOT_FOUND;
    }

    node->file_content = data;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::append (const std::string& agent_id, const std::string& handle, const std::string& data) {
    FileHandle* file_handle = resolve_handle (agent_id, handle);
    if (file_handle == nullptr) {
        return VFSResult::INVALID_HANDLE;
    }

    if (file_handle->mode == "r") {
        std::cerr << "Nije dozvoljeno dodavanje u fajl otvoren za čitanje: " << file_handle->vfs_path << std::endl;
        return VFSResult::PERMISSION_DENIED;
    }

    VFSNode* node = resolve_file_node (file_handle->vfs_path);
    if (node == nullptr) {
        return VFSResult::NOT_FOUND;
    }

    node->file_content += data;
    return VFSResult::OK;
}

VFSResult InMemoryVFS::close (const std::string& agent_id, const std::string& handle) {
    FileHandle* file_handle = resolve_handle (agent_id, handle);
    if (file_handle == nullptr) {
        return VFSResult::INVALID_HANDLE;
    }

    if (file_handle->mode == "r") {
        if (!locks[file_handle->vfs_path].unlock_read (agent_id)) {
            std::cerr << "Nije dozvoljen pristup fajlu: " << file_handle->vfs_path << std::endl;
            return VFSResult::PERMISSION_DENIED;
        }
    } else {
        if (!locks[file_handle->vfs_path].unlock_write (agent_id)) {
            std::cerr << "Nije dozvoljen pristup fajlu: " << file_handle->vfs_path << std::endl;
            return VFSResult::PERMISSION_DENIED;
        }
    }

    auto agent_it = agent_handles.find (agent_id);
    if (agent_it != agent_handles.end ()) {
        agent_it->second.erase (handle);
    }

    return VFSResult::OK;
}

void InMemoryVFS::dump (std::ostream& out) const {
    for (const auto& [path, node] : nodes) {
        out << path << " (" << (node.type == VFSNodeType::FILE ? "FILE" : "DIRECTORY") << ") "
            << "[" << (node.is_read_only ? "RO" : "RW") << "]\n";
    }
}