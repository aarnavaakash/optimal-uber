#include "cli.h"
#include "tests.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Check if the user requested to run tests directly
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--run-tests" || arg == "-t") {
            bool passed = run_all_tests();
            return passed ? 0 : 1;
        } else {
            std::cout << "Usage:\n"
                      << "  " << argv[0] << "              Launch interactive CLI\n"
                      << "  " << argv[0] << " --run-tests   Run automated unit tests\n";
            return 1;
        }
    }

    // Launch the interactive terminal console
    InteractiveCommandLineInterface cli;
    cli.run();

    return 0;
}
