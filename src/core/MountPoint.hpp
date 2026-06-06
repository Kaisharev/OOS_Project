struct MountPoint {
        std::string source;
        std::string target;
        enum class Mode { RO, RW } mode;
};
