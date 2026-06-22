#pragma once
#include <string>
enum class OperationType { THINK, OPEN, READ, WRITE, CLOSE, APPEND };
/*
    OperationType type; - THINK, OPEN, READ, WRITE, CLOSE, APPEND
    int thinking_duration; - trajanje operacije THINK
    std::string path; - putanja fajla za OPEN, WRITE, APPEND
    std::string data; - podaci za WRITE i APPEND
    std::string mode; - režim za OPEN (read/write)
    std::string handle; - ime handle-a za OPEN, READ, WRITE, CLOSE, APPEND
    */
class Operation {
    private:
        OperationType type;
        int thinking_duration;
        std::string path;
        std::string data;
        std::string mode;
        std::string handle;

    public:
        Operation (OperationType t = OperationType::THINK, int duration = -1, const std::string& p = "", const std::string& d = "",
                   const std::string& m = "", const std::string& h = "")
            : type (t), thinking_duration (duration), path (p), data (d), mode (m), handle (h) {}

        OperationType getType () const {
            return type;
        }
        int getThinkingDuration () const {
            return thinking_duration;
        }
        const std::string& getPath () const {
            return path;
        }
        const std::string& getData () const {
            return data;
        }
        const std::string& getMode () const {
            return mode;
        }
        const std::string& getHandle () const {
            return handle;
        }
};
