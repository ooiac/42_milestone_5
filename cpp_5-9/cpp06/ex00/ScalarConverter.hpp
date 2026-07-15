//
// Created by caida-si on 15/07/2026.
//`

#ifndef SCALARCONVERTER_HPP
#define SCALARCONVERTER_HPP

#include <string>

class ScalarConverter {
private:
	ScalarConverter();
	ScalarConverter(const ScalarConverter& other);
	ScalarConverter& operator=(const ScalarConverter& other);
	~ScalarConverter();

	static bool isChar(const std::string& literal);
	static bool isInt(const std::string& literal);
	static bool isFloat(const std::string& literal);
	static bool isDouble(const std::string& literal);
	static bool isPseudoLiteral(const std::string& literal);

	static void convertFromChar(const std::string& literal);
	static void convertFromInt(const std::string& literal);
	static void convertFromFloat(const std::string& literal);
	static void convertFromDouble(const std::string& literal);
	static void convertPseudoLiteral(const std::string& literal);

public:
	static void convert(const std::string& literal);
};

#endif