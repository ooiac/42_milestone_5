//
// Created by caida-si on 16/07/26.
//

#include "BitcoinExchange.hpp"
#include <iostream>

int main(int ac, char **av) {
    if (ac != 2) {
        std::cerr << "Error: could not open file." << std::endl;
        return 1;
    }

    BitcoinExchange exchanger;
    if (!exchanger.loadDatabase("data.csv")) {
        std::cerr << "Error: could not load database." << std::endl;
        return 1;
    }

    exchanger.processInput(av[1]);
    return 0;
}