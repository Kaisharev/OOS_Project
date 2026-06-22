#include "AgentParser.hpp"
/*
      class Operation {
  private:
      OperationType type;
      int thinking_duration;
      std::string path;
      std::string data;
      std::string mode;
      std::string handle;
  }

      THINK <duration> - Operation (THINK, 3,,,,)
      OPEN <path> <mode> as <handle> - Operation (OPEN, ,path,,write,handle)
      READ <handle> - Operation (READ,,,,handle)
      WRITE <handle> <data> - Operation (WRITE,,,data,,handle)
      CLOSE <handle> - Operation (CLOSE,,,,,handle)
      APPEND <handle> <data> - Operation (APPEND,,,data,,handle)

  */

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

        auto tokens = tokenize (line);

        if (tokens.empty ()) continue;

        /* Mora postojati bolji način da se implementira parser, ali vrijeme je ograničavajući faktor... */

        if (tokens[0] == "THINK") {
            if (tokens.size () != 2) {
                throw std::runtime_error ("Operacije THINK zahtjeva 1 argument, a uneseni su " + std::to_string (tokens.size ()));
            }
            agent.ops.emplace_back (OperationType::THINK, std::stoi (std::string (tokens[1])), "", "", "", "");
        } else if (tokens[0] == "OPEN") {
            if (tokens.size () != 5 || std::string (tokens[3]) != "as") {
                throw std::runtime_error ("Operacija OPEN nije ispravno definisana.");
            }
            agent.ops.emplace_back (OperationType::OPEN, 0, std::string (tokens[1]), "", std::string (tokens[2]),
                                    std::string (tokens[4]));
        } else if (tokens[0] == "READ") {
            if (tokens.size () != 2) {
                throw std::runtime_error ("Operacije READ zahtjeva 1 argument, a uneseni su " + std::to_string (tokens.size ()));
            }
            agent.ops.emplace_back (OperationType::READ, 0, "", "", "", std::string (tokens[1]));
        } else if (tokens[0] == "WRITE") {
            if (tokens.size () != 3) {
                throw std::runtime_error ("Operacije WRITE zahtjeva 2 argumenta, a uneseni su " + std::to_string (tokens.size ()));
            }
            agent.ops.emplace_back (OperationType::WRITE, 0, "", std::string (tokens[2]), "", std::string (tokens[1]));
        } else if (tokens[0] == "CLOSE") {
            if (tokens.size () != 2) {
                throw std::runtime_error ("Operacije CLOSE zahtjeva 1 argument, a uneseni su " + std::to_string (tokens.size ()));
            }
            agent.ops.emplace_back (OperationType::CLOSE, 0, "", "", "", std::string (tokens[1]));
        } else if (tokens[0] == "APPEND") {
            if (tokens.size () != 3) {
                throw std::runtime_error ("Operacije APPEND zahtjeva 2 argumenta, a uneseni su " + std::to_string (tokens.size ()));
            }
            agent.ops.emplace_back (OperationType::APPEND, 0, "", std::string (tokens[2]), "", std::string (tokens[1]));
        } else {
            throw std::runtime_error ("Nepoznata operacija: " + std::string (tokens[0]));
        }
    }
    return agent;
}