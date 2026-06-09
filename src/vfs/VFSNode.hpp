#pragma once

enum class VFSNodeType { FILE, DIRECTORY };
#include <string>
#include <vector>

struct VFSNode {
        std::string name;
        VFSNodeType type;
        std::string file_content;  // sadržaj fajla
        bool is_read_only = false;
};