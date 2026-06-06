#pragma once
struct FileHandle {
        std::string vfs_path;
        std::string mode;  // "read"/"write"/"append"
        bool is_open = false;
};