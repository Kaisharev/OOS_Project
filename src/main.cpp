#include <iostream>
#include <stdexcept>

#include "core/Config.hpp"
#include "simulation/Simulator.hpp"

#ifdef _WIN32
#    include <windows.h>
#endif

int main (int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP (CP_UTF8);
#endif

    if (argc < 2) {
        std::cerr << "Upotreba: simulator <config.json>\n" << std::flush;
        return 1;
    }

    try {
        Config config = Config::load_from_file (argv[1]);
        Simulator sim (config);
        sim.run ();
    } catch (const std::exception& e) {
        std::cerr << "\nGreska: " << e.what () << "\n" << std::flush;
        return 1;
    } catch (...) {
        std::cerr << "\nNepoznata greska!\n" << std::flush;
        return 1;
    }

    return 0;
}
