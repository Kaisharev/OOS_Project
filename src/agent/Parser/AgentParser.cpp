#include "AgentParser.hpp"

std::vector<std::string_view> AgentParser::tokenize (std::string_view line) {
    std::vector<std::string_view> parts;

    while (!line.empty ()) {
        const auto start = line.find_first_not_of (" \t");
        if (start == std::string_view::npos) break;
        line.remove_prefix (start);

        if (line.front () == '"') {
            line.remove_prefix (1);
            const auto closing_quote = line.find ('"');
            if (closing_quote == std::string_view::npos) {
                parts.push_back (line);
                break;
            } else {
                parts.push_back (line.substr (0, closing_quote));
                line.remove_prefix (closing_quote + 1);
            }
        } else {
            const auto end = line.find_first_of (" \t");
            parts.push_back (line.substr (0, end));
            if (end == std::string_view::npos) break;
            line.remove_prefix (end);
        }
    }
    return parts;
}

Agent AgentParser::Parse (const AgentConfig& config) {
    Agent agent (config.id, config.priority, config.arrival_time);

    std::ifstream script_file (config.script_path);
    if (!script_file.is_open ()) {
        throw std::runtime_error ("Could not open script file: " + config.script_path);
    }

    std::string line;
    while (std::getline (script_file, line)) {
        if (line.empty ()) continue;
        // Ukloni Windows \r ako postoji
        if (!line.empty () && line.back () == '\r') line.pop_back ();
        if (line.empty ()) continue;

        auto tokens = tokenize (line);
        if (tokens.empty ()) continue;

        if (tokens[0] == "THINK") {
            if (tokens.size () != 2)
                throw std::runtime_error ("THINK zahtjeva 1 argument");
            agent.addOp (Operation (OperationType::THINK, std::stoi (std::string (tokens[1])), "", "", "", ""));

        } else if (tokens[0] == "OPEN") {
            if (tokens.size () != 5 || std::string (tokens[3]) != "as")
                throw std::runtime_error ("OPEN: očekivano: OPEN <putanja> <mod> as <handle>");

            std::string mode = std::string (tokens[2]);
            if      (mode == "r")      mode = "read";
            else if (mode == "w")      mode = "write";
            else if (mode == "a")      mode = "append";
            else if (mode != "read" && mode != "write" && mode != "append")
                throw std::runtime_error ("Neispravan mode za OPEN: " + mode);

            agent.addOp (Operation (OperationType::OPEN, 0,
                                    std::string (tokens[1]), "",
                                    mode, std::string (tokens[4])));

        } else if (tokens[0] == "READ") {
            if (tokens.size () != 2)
                throw std::runtime_error ("READ zahtjeva 1 argument");
            agent.addOp (Operation (OperationType::READ, 0, "", "", "", std::string (tokens[1])));

        } else if (tokens[0] == "WRITE") {
            if (tokens.size () != 3)
                throw std::runtime_error ("WRITE zahtjeva 2 argumenta");
            agent.addOp (Operation (OperationType::WRITE, 0, "",
                                    std::string (tokens[2]), "", std::string (tokens[1])));

        } else if (tokens[0] == "CLOSE") {
            if (tokens.size () != 2)
                throw std::runtime_error ("CLOSE zahtjeva 1 argument");
            agent.addOp (Operation (OperationType::CLOSE, 0, "", "", "", std::string (tokens[1])));

        } else if (tokens[0] == "APPEND") {
            if (tokens.size () != 3)
                throw std::runtime_error ("APPEND zahtjeva 2 argumenta");
            agent.addOp (Operation (OperationType::APPEND, 0, "",
                                    std::string (tokens[2]), "", std::string (tokens[1])));

        } else {
            throw std::runtime_error ("Nepoznata operacija: " + std::string (tokens[0]));
        }
    }
    return agent;
}
