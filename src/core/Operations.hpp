enum class OperationType {
    THINK,
    OPEN,
    READ,
    WRITE,
    CLOSE,
    APPEND
}

class Operation {
    private:
        OperationType type;
        int thinking_duration;
        std::string path;
        std::string data;
        std::string mode;
        std::string handle;
}