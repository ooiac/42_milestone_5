#include <iostream>
#include <csignal>
#include "Config.hpp"
#include "Server.hpp"

static volatile bool g_running = true;

static void sigHandler(int sig) {
    (void)sig;
    g_running = false;
    std::cout << "\nShutting down..." << std::endl;
    // exit cleanly
    _exit(0);
}

int main(int argc, char **argv) {
    std::string configFile = "webserv.conf";
    if (argc == 2) {
        configFile = argv[1];
    } else if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT,  sigHandler);
    signal(SIGTERM, sigHandler);

    Config cfg;
    if (!cfg.parse(configFile)) {
        std::cerr << "Failed to parse config: " << configFile << std::endl;
        return 1;
    }

    Server server;
    if (!server.init(cfg)) {
        std::cerr << "Failed to initialize server" << std::endl;
        return 1;
    }

    std::cout << "webserv started. Press Ctrl+C to stop." << std::endl;
    server.run();

    return 0;
}
