//
// Created by caida-si on 5/7/26.
//

#include "RobotomyRequestForm.hpp"

RobotomyRequestForm::RobotomyRequestForm(const std::string target)
	: AForm("RobotomyRequestForm", 72, 45), _target(target)
{
}

RobotomyRequestForm::RobotomyRequestForm(const RobotomyRequestForm &other)
	: AForm(other), _target(other._target)
{
}

RobotomyRequestForm &RobotomyRequestForm::operator=(const RobotomyRequestForm &other)
{
	if (this != &other)
	{
		AForm::operator=(other);
		_target = other._target;
	}
	return (*this);
}

RobotomyRequestForm::~RobotomyRequestForm()
{
}

void RobotomyRequestForm::executeForm(const Bureaucrat &executor) const
{
	std::cout << "* BZZZZ *  * BZZZZ * *  BZZZZ *" << std::endl;

	srand(time(NULL));
	if (rand() % 2 == 0)
	{
		std::cout << _target << " has been successfully robotomized :)" << std::endl;
	}
	else
	{
		std::cout << _target << " robotomy failed." << std::endl;
	}

	std::cout << executor.getName() << " executed " << getName() << std::endl;
}