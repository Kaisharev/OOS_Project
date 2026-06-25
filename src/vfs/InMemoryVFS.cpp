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

    bool is_ro = (mount_point.mode == MountPoint::Mode::RO);

    if (std::filesystem::is_regular_file (mount_path)) {
        // Direktno mountovanje jednog fajla
        std::string content = read_content (mount_path);
        nodes[mount_point.target] = VFSNode{mount_path.filename ().string (), VFSNodeType::FILE, content, is_ro};
        return VFSResult::OK;
    }

    if (!std::filesystem::is_directory (mount_path)) {
        std::cerr << "Mount path nije direktorij ni fajl: " << mount_path << std::endl;
        return VFSResult::NOT_FOUND;
    }

    nodes[mount_point.target] =
        VFSNode{mount_path.filename ().string (), VFSNodeType::DIRECTORY, "", is_ro};

    for (const auto& entry : std::filesystem::recursive_directory_iterator (mount_path)) {
        std::string relative_path = std::filesystem::relative (entry.path (), mount_path).string ();
        // Normalizuj separatore na /
        for (char& c : relative_path) {
            if (c == '\\') c = '/';
        }
        std::string vfs_path = mount_point.target + "/" + relative_path;

        if (entry.is_regular_file ()) {
            std::string content = read_content (entry.path ());
            nodes[vfs_path] = VFSNode{relative_path, VFSNodeType::FILE, content, is_ro};
        } else if (entry.is_directory ()) {
            nodes[vfs_path] = VFSNode{relative_path, VFSNodeType::DIRECTORY, "", is_ro};
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
        return nullptr;
    }
    auto handle_it = agent_it->second.find (handle);
    if (handle_it == agent_it->second.end ()) {
        return nullptr;
    }
    return &handle_it->second;
}

VFSResult InMemoryVFS::open (const std::string& agent_id, const std::string& path, const std::string& mode,
                             const std::string& handle) {
    VFSNode* node = resolve_path (path);
    if (node == nullptr) {
        return VFSResult::NOT_FOUND;
    }
    if (mode != "read" && mode != "write" && mode != "append") {
        return VFSResult::INVALID_MODE;
    }
    if (node->type == VFSNodeType::DIRECTORY) {
        return VFSResult::NOT_FOUND;
    }
    if (node->is_read_only && (mode == "write" || mode == "append")) {
        return VFSResult::PERMISSION_DENIED;
    }
    if (agent_handles[agent_id].find (handle) != agent_handles[agent_id].end ()) {
        return VFSResult::INVALID_HANDLE;
    }

    bool locked = false;
    if (mode == "read") {
        locked = locks[path].try_lock_read (agent_id);
    } else {
        // write i append zahtijevaju ekskluzivan pristup (write lock)
        locked = locks[path].try_lock_write (agent_id);
    }

    if (!locked) {
        return VFSResult::WOULD_BLOCK;
    }

    agent_handles[agent_id][handle] = FileHandle{path, mode, true};
    return VFSResult::OK;
}

VFSResult InMemoryVFS::read (const std::string& agent_id, const std::string& handle, std::string& out_content) {
    FileHandle* file_handle = resolve_handle (agent_id, handle);
    if (file_handle == nullptr) {
        return VFSResult::INVALID_HANDLE;
    }
    if (file_handle->mode != "read") {
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
    if (file_handle->mode == "read") {
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
    if (file_handle->mode == "read") {
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

    if (file_handle->mode == "read") {
        if (!locks[file_handle->vfs_path].unlock_read (agent_id)) {
            return VFSResult::PERMISSION_DENIED;
        }
    } else {
        if (!locks[file_handle->vfs_path].unlock_write (agent_id)) {
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
    out << "=== Završno stanje VFS-a ===\n";
    // Sortirano po putanji radi čitljivosti
    std::vector<std::pair<std::string, const VFSNode*>> sorted_nodes;
    for (const auto& [path, node] : nodes) {
        sorted_nodes.emplace_back (path, &node);
    }
    std::sort (sorted_nodes.begin (), sorted_nodes.end (),
               [] (const auto& a, const auto& b) { return a.first < b.first; });

    for (const auto& [path, node] : sorted_nodes) {
        if (node->type == VFSNodeType::FILE) {
            out << path << ": \"" << node->file_content << "\"\n";
        }
    }
}

std::string InMemoryVFS::get_lock_holder (const std::string& path) const {
    auto it = locks.find (path);
    if (it == locks.end ()) return "";
    return it->second.get_holder ();
}
