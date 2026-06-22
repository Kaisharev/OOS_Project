#include <iostream>
#include <stdexcept>

#include "core/Config.hpp"
#include "simulation/Simulator.hpp"

int main (int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Upotreba: simulator <config.json>\n";
        return 1;
    }

    try {
        Config config = Config::load_from_file (argv[1]);
        Simulator sim (config);
        sim.run ();
    } catch (const std::exception& e) {
        std::cerr << "Greška: " << e.what () << "\n";
        return 1;
    }

    return 0;
}