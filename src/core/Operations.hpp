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
/*
    OperationType type; - THINK, OPEN, READ, WRITE, CLOSE, APPEND
    int thinking_duration; - trajanje operacije THINK
    std::string path; - putanja fajla za OPEN, WRITE, APPEND
    std::string data; - podaci za WRITE i APPEND
    std::string mode; - režim za OPEN (read/write/append)
    std::string handle; - ime handle-a za OPEN, READ, WRITE, CLOSE, APPEND
    */