//
// Created by caida-si on 5/7/26.
//

#include "Bureaucrat.hpp"
#include "Form.hpp"

int main(void)
{
	try
	{
		Bureaucrat john("John", 5);
		std::cout << john << std::endl;

		Form f1("TaxForm", 10, 20);
		std::cout << f1 << std::endl;

		john.signForm(f1);
		std::cout << f1 << std::endl;

		Bureaucrat bob("Bob", 50);
		Form f2("SecurityForm", 20, 40);

		bob.signForm(f2);
		std::cout << f2 << std::endl;
	}
	catch (std::exception &e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
	}

	return (0);
}