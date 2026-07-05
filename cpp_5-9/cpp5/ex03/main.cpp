//
// Created by caida-si on 5/7/26.
//

#include "Bureaucrat.hpp"
#include "AForm.hpp"
#include "Intern.hpp"

int main(void)
{
	Bureaucrat god("God", 1);

	Intern someIntern;

	try
	{
		AForm *form1 = someIntern.makeForm("shrubbery creation", "Garden");
		god.signForm(*form1);
		god.executeForm(*form1);
		delete form1;

		std::cout << std::endl;

		AForm *form2 = someIntern.makeForm("robotomy request", "Bender");
		god.signForm(*form2);
		god.executeForm(*form2);
		delete form2;

		std::cout << std::endl;

		AForm *form3 = someIntern.makeForm("presidential pardon", "Criminal");
		god.signForm(*form3);
		god.executeForm(*form3);
		delete form3;

		std::cout << std::endl;

		// This should throw
		AForm *form4 = someIntern.makeForm("invalid form", "Target");
		delete form4;
	}
	catch (std::exception &e)
	{
		std::cout << "Exception caught: " << e.what() << std::endl;
	}

	return (0);
}