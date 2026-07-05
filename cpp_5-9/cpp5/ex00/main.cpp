//
// Created by caida-si on 5/7/26.
//

#include "Bureaucrat.hpp"

int main(void)
{
	try
	{
		Bureaucrat john("John", 42);
		std::cout << john << std::endl;

		john.incrementGrade();
		std::cout << john << std::endl;

		john.decrementGrade();
		std::cout << john << std::endl;

		// Test grade boundaries
		Bureaucrat almostGod("AlmostGod", 2);
		almostGod.incrementGrade();
		std::cout << almostGod << std::endl;

		// This will throw
		almostGod.incrementGrade();
	}
	catch (std::exception &e)
	{
		std::cout << "Exception caught: " << e.what() << std::endl;
	}

	try
	{
		Bureaucrat almostDead("AlmostDead", 149);
		almostDead.decrementGrade();
		std::cout << almostDead << std::endl;

		// This will throw
		almostDead.decrementGrade();
	}
	catch (std::exception &e)
	{
		std::cout << "Exception caught: " << e.what() << std::endl;
	}

	try
	{
		// This will throw during construction
		Bureaucrat invalid("Invalid", 151);
	}
	catch (std::exception &e)
	{
		std::cout << "Exception caught: " << e.what() << std::endl;
	}

	return (0);
}