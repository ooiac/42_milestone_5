//
// Created by caida-si on 5/7/26.
//

#include "ShrubberyCreationForm.hpp"

ShrubberyCreationForm::ShrubberyCreationForm(const std::string target)
	: AForm("ShrubberyCreationForm", 145, 137), _target(target)
{
}

ShrubberyCreationForm::ShrubberyCreationForm(const ShrubberyCreationForm &other)
	: AForm(other), _target(other._target)
{
}

ShrubberyCreationForm &ShrubberyCreationForm::operator=(const ShrubberyCreationForm &other)
{
	if (this != &other)
	{
		AForm::operator=(other);
		_target = other._target;
	}
	return (*this);
}

ShrubberyCreationForm::~ShrubberyCreationForm()
{
}

void ShrubberyCreationForm::executeForm(const Bureaucrat &executor) const
{
	std::string filename = _target + "_shrubbery";
	std::ofstream file(filename.c_str());

	if (!file.is_open())
	{
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}

	// ANSI color codes
	std::string green = "\033[32m";
	std::string darkGreen = "\033[38;5;22m";
	std::string lightGreen = "\033[92m";
	std::string brown = "\033[38;5;94m";
	std::string darkBrown = "\033[38;5;58m";
	std::string reset = "\033[0m";

	file << green << "                                                         ." << reset << std::endl;
	file << green << "                                              .         ;  " << reset << std::endl;
	file << lightGreen << "                 .              .              ;%     ;;" << reset << std::endl;
	file << lightGreen << "                   ,           ,                :;%  %;" << reset << std::endl;
	file << green << "                    :         ;                   :;%;'     .," << reset << std::endl;
	file << green << "           ,.        %;     %;            ;        %;'    ,;" << reset << std::endl;
	file << darkGreen << "             ;       ;%;  %%;        ,     %;    ;%;    ,%'" << reset << std::endl;
	file << darkGreen << "              %;       %;%;      ,  ;       %;  ;%;   ,%;'" << reset << std::endl;
	file << green << "               ;%;      %;        ;%;        % ;%;  ,%;'" << reset << std::endl;
	file << lightGreen << "                `%;.     ;%;     %;'         `;%%;.%;'" << reset << std::endl;
	file << green << "                 `:;%.    ;%%. %@;        %; ;@%;%'" << reset << std::endl;
	file << darkGreen << "                    `:%;.  :;bd%;          %;@%;'" << reset << std::endl;
	file << green << "                      `@%:.  :;%.         ;@@%;'" << reset << std::endl;
	file << darkGreen << "                        `@%.  `;@%.      ;@@%;" << reset << std::endl;
	file << green << "                          `@%%. `@%%    ;@@%;" << reset << std::endl;
	file << darkGreen << "                            ;@%. :@%%  %@@%;" << reset << std::endl;
	file << brown << "                              %@bd%%%bd%%:;" << reset << std::endl;
	file << brown << "                                #@%%%%%:;;" << reset << std::endl;
	file << brown << "                                %@@%%%::;" << reset << std::endl;
	file << brown << "                                %@@@@%(o);  . '" << reset << std::endl;
	file << darkBrown << "                                %@@@o%;:(.,'         " << reset << std::endl;
	file << darkBrown << "                            `.. %@@@o%::;         " << reset << std::endl;
	file << brown << "                               `)@@@o%::;         " << reset << std::endl;
	file << darkBrown << "                                %@@(o)::;        " << reset << std::endl;
	file << brown << "                               .%@@@@%::;         " << reset << std::endl;
	file << brown << "                               ;%@@@@%::;.          " << reset << std::endl;
	file << darkBrown << "                              ;%@@@@%%:;;;. " << reset << std::endl;
	file << darkBrown << "                          ...;%@@@@@%%:;;;;,.." << reset << std::endl;

	file.close();

	std::cout << executor.getName() << " executed " << getName() << std::endl;
}