//
// Created by caida-si on 5/7/26.
//

#include "Form.hpp"

// Exception implementations
const char *Form::GradeTooHighException::what() const throw()
{
	return "Grade is too high!";
}

const char *Form::GradeTooLowException::what() const throw()
{
	return "Grade is too low!";
}

// Form constructors and destructors
Form::Form(const std::string name, int signGrade, int execGrade)
	: _name(name), _isSigned(false), _signGrade(signGrade), _execGrade(execGrade)
{
	if (signGrade < 1 || execGrade < 1)
		throw GradeTooHighException();
	if (signGrade > 150 || execGrade > 150)
		throw GradeTooLowException();
}

Form::Form(const Form &other)
	: _name(other._name), _isSigned(other._isSigned),
	  _signGrade(other._signGrade), _execGrade(other._execGrade)
{
}

Form &Form::operator=(const Form &other)
{
	if (this != &other)
	{
		_isSigned = other._isSigned;
	}
	return (*this);
}

Form::~Form()
{
}

// Getters
const std::string Form::getName() const
{
	return (_name);
}

bool Form::getIsSigned() const
{
	return (_isSigned);
}

int Form::getSignGrade() const
{
	return (_signGrade);
}

int Form::getExecGrade() const
{
	return (_execGrade);
}

// Signing logic
void Form::beSigned(const Bureaucrat &bureaucrat)
{
	if (bureaucrat.getGrade() > _signGrade)
		throw GradeTooLowException();
	_isSigned = true;
}

// Output operator
std::ostream &operator<<(std::ostream &os, const Form &form)
{
	os << "Form: " << form.getName()
	   << ", Signed: " << (form.getIsSigned() ? "yes" : "no")
	   << ", Sign Grade: " << form.getSignGrade()
	   << ", Exec Grade: " << form.getExecGrade();
	return (os);
}