#include <vector>
#include <string>
#include <chrono>
enum class AgentState { READY, RUNNING, BLOCKED, DONE };

class Agent {
    private:
        std::vector<std::string> operations;
        const auto start{std::chrono::steady_clock::now()};
        int priority;
        AgentState state = AgentState::READY;
        
        int start_time = -1, end_time = -1;
        int wait_time = 0, blocked_time = 0;
        int preemptions = 0;

        std::unordered_map<std::string, FileHandle> handles;

};