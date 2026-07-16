//
// Created by caida-si on 16/07/26.
//

#include "RPN.hpp"
#include <iostream>

int main(int ac, char **av)
{
	if (ac != 2) {
		std::cerr << "Error" << std::endl;
		return 1;
	}

	RPN rpn;
	if (!rpn.evaluate(av[1])) {
		std::cerr << "Error" << std::endl;
		return 1;
	}

	std::cout << rpn.getResult() << std::endl;
	return 0;
}