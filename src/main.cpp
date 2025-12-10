#include "core/Application.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: terram <game_directory>" << std::endl;
        std::cerr << "Example: terram ./examples/hello" << std::endl;
        return 1;
    }

    terram::Application app;

    if (!app.init(argv[1])) {
        std::cerr << "Failed to initialize Terram" << std::endl;
        return 1;
    }

    app.run();

    return 0;
}
