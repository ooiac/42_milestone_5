//
// Created by caida-si on 5/7/26.
//

#include "Bureaucrat.hpp"
#include "AForm.hpp"
#include "ShrubberyCreationForm.hpp"
#include "RobotomyRequestForm.hpp"
#include "PresidentialPardonForm.hpp"

int main(void)
{
	try
	{
		Bureaucrat god("God", 1);
		Bureaucrat poor("PoorGuy", 146);

		ShrubberyCreationForm shrubbery("garden");
		RobotomyRequestForm robotomy("Bender");
		PresidentialPardonForm pardon("Criminal");

		std::cout << "=== Testing Shrubbery ===" << std::endl;
		god.signForm(shrubbery);
		god.executeForm(shrubbery);

		std::cout << "\n=== Testing Robotomy ===" << std::endl;
		god.signForm(robotomy);
		god.executeForm(robotomy);

		std::cout << "\n=== Testing Presidential Pardon ===" << std::endl;
		god.signForm(pardon);
		god.executeForm(pardon);

		std::cout << "\n=== Testing insufficient grade ===" << std::endl;
		poor.signForm(shrubbery);
		poor.executeForm(shrubbery);
	}
	catch (std::exception &e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
	}

	return (0);
}