//
// Created by caida-si on 5/7/26.
//

#ifndef INTERN_HPP
#define INTERN_HPP

#include <iostream>
#include <string>
#include "AForm.hpp"

class Intern
{
private:
	AForm	*createShrubberyForm(const std::string &target);
	AForm	*createRobotomyForm(const std::string &target);
	AForm	*createPresidentialForm(const std::string &target);

public:
	Intern();
	Intern(const Intern &other);
	Intern &operator=(const Intern &other);
	~Intern();

	AForm	*makeForm(const std::string &formName, const std::string &target);

	class UnknownFormException : public std::exception
	{
	public:
		virtual const char *what() const throw();
	};
};

#endif