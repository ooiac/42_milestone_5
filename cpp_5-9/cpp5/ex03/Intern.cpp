//
// Created by caida-si on 5/7/26.
//

#include "Intern.hpp"
#include "ShrubberyCreationForm.hpp"
#include "RobotomyRequestForm.hpp"
#include "PresidentialPardonForm.hpp"

// Exception implementation
const char *Intern::UnknownFormException::what() const throw()
{
	return "Unknown form type!";
}

// Constructors and destructors
Intern::Intern()
{
}

Intern::Intern(const Intern &other)
{
	(void)other;
}

Intern &Intern::operator=(const Intern &other)
{
	if (this != &other)
	{
	}
	return (*this);
}

Intern::~Intern()
{
}

// Private form creation methods
AForm *Intern::createShrubberyForm(const std::string &target)
{
	return (new ShrubberyCreationForm(target));
}

AForm *Intern::createRobotomyForm(const std::string &target)
{
	return (new RobotomyRequestForm(target));
}

AForm *Intern::createPresidentialForm(const std::string &target)
{
	return (new PresidentialPardonForm(target));
}

// Factory method with function pointer approach (cleaner than massive if/else)
AForm *Intern::makeForm(const std::string &formName, const std::string &target)
{
	struct FormFactory
	{
		const char		*name;
		AForm			*(Intern::*create)(const std::string &target);
	};

	FormFactory factories[] = {
		{"shrubbery creation", &Intern::createShrubberyForm},
		{"robotomy request", &Intern::createRobotomyForm},
		{"presidential pardon", &Intern::createPresidentialForm},
		{NULL, NULL}
	};

	for (int i = 0; factories[i].name != NULL; i++)
	{
		if (formName == factories[i].name)
		{
			std::cout << "Intern creates " << formName << std::endl;
			return ((this->*(factories[i].create))(target));
		}
	}

	throw UnknownFormException();
}