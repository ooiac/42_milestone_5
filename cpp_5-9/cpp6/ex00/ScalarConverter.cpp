//
// Created by caida-si on 15/07/2026.
//

#include "ScalarConverter.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <cerrno>
#include <limits>
#include <iomanip>

ScalarConverter::ScalarConverter() {}
ScalarConverter::ScalarConverter(const ScalarConverter& other) { (void)other; }
ScalarConverter& ScalarConverter::operator=(const ScalarConverter& other) { (void)other; return *this; }
ScalarConverter::~ScalarConverter() {}

bool ScalarConverter::isChar(const std::string& literal) {
    return (literal.length() == 3 && literal[0] == '\'' && literal[2] == '\'');
}

bool ScalarConverter::isInt(const std::string& literal) {
    if (literal.empty()) return false;

    size_t i = 0;
    if (literal[i] == '+' || literal[i] == '-')
        i++;

    if (i >= literal.length()) return false;

    while (i < literal.length()) {
        if (!isdigit(literal[i]))
            return false;
        i++;
    }
    return true;
}

bool ScalarConverter::isFloat(const std::string& literal) {
    if (literal.empty() || literal[literal.length() - 1] != 'f')
        return false;

    std::string num = literal.substr(0, literal.length() - 1);
    if (num.empty()) return false;

    size_t i = 0;
    bool hasDot = false;

    if (num[i] == '+' || num[i] == '-')
        i++;

    if (i >= num.length()) return false;

    while (i < num.length()) {
        if (num[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!isdigit(num[i])) {
            return false;
        }
        i++;
    }
    return hasDot;
}

bool ScalarConverter::isDouble(const std::string& literal) {
    if (literal.empty()) return false;

    size_t i = 0;
    bool hasDot = false;

    if (literal[i] == '+' || literal[i] == '-')
        i++;

    if (i >= literal.length()) return false;

    while (i < literal.length()) {
        if (literal[i] == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!isdigit(literal[i])) {
            return false;
        }
        i++;
    }
    return hasDot;
}

bool ScalarConverter::isPseudoLiteral(const std::string& literal) {
    return (literal == "nan" || literal == "nanf" ||
            literal == "+inf" || literal == "-inf" ||
            literal == "+inff" || literal == "-inff");
}

void ScalarConverter::convertFromChar(const std::string& literal) {
    char c = literal[1];

    std::cout << "char: '" << c << "'" << std::endl;
    std::cout << "int: " << static_cast<int>(c) << std::endl;
    std::cout << "float: " << std::fixed << std::setprecision(1) << static_cast<float>(c) << "f" << std::endl;
    std::cout << "double: " << std::fixed << std::setprecision(1) << static_cast<double>(c) << std::endl;
}

void ScalarConverter::convertFromInt(const std::string& literal) {
    errno = 0;
    char* end;
    long value = strtol(literal.c_str(), &end, 10);

    if (errno == ERANGE || value > INT_MAX || value < INT_MIN) {
        std::cout << "char: impossible" << std::endl;
        std::cout << "int: impossible" << std::endl;
        std::cout << "float: impossible" << std::endl;
        std::cout << "double: impossible" << std::endl;
        return;
    }

    int intValue = static_cast<int>(value);

    if (intValue < 32 || intValue > 126)
        std::cout << "char: Non displayable" << std::endl;
    else
        std::cout << "char: '" << static_cast<char>(intValue) << "'" << std::endl;

    std::cout << "int: " << intValue << std::endl;
    std::cout << "float: " << std::fixed << std::setprecision(1) << static_cast<float>(intValue) << "f" << std::endl;
    std::cout << "double: " << std::fixed << std::setprecision(1) << static_cast<double>(intValue) << std::endl;
}

void ScalarConverter::convertFromFloat(const std::string& literal) {
    std::string num = literal.substr(0, literal.length() - 1);
    float floatValue = static_cast<float>(atof(num.c_str()));

    if (floatValue < 0 || floatValue > 127 || std::isnan(floatValue) || std::isinf(floatValue))
        std::cout << "char: impossible" << std::endl;
    else if (floatValue < 32 || floatValue > 126)
        std::cout << "char: Non displayable" << std::endl;
    else
        std::cout << "char: '" << static_cast<char>(floatValue) << "'" << std::endl;

    if (std::isnan(floatValue) || std::isinf(floatValue) ||
        floatValue > INT_MAX || floatValue < INT_MIN)
        std::cout << "int: impossible" << std::endl;
    else
        std::cout << "int: " << static_cast<int>(floatValue) << std::endl;

    std::cout << "float: " << std::fixed << std::setprecision(1) << floatValue << "f" << std::endl;
    std::cout << "double: " << std::fixed << std::setprecision(1) << static_cast<double>(floatValue) << std::endl;
}

void ScalarConverter::convertFromDouble(const std::string& literal) {
    double doubleValue = atof(literal.c_str());

    if (doubleValue < 0 || doubleValue > 127 || std::isnan(doubleValue) || std::isinf(doubleValue))
        std::cout << "char: impossible" << std::endl;
    else if (doubleValue < 32 || doubleValue > 126)
        std::cout << "char: Non displayable" << std::endl;
    else
        std::cout << "char: '" << static_cast<char>(doubleValue) << "'" << std::endl;

    if (std::isnan(doubleValue) || std::isinf(doubleValue) ||
        doubleValue > INT_MAX || doubleValue < INT_MIN)
        std::cout << "int: impossible" << std::endl;
    else
        std::cout << "int: " << static_cast<int>(doubleValue) << std::endl;

    std::cout << "float: " << std::fixed << std::setprecision(1) << static_cast<float>(doubleValue) << "f" << std::endl;
    std::cout << "double: " << std::fixed << std::setprecision(1) << doubleValue << std::endl;
}

void ScalarConverter::convertPseudoLiteral(const std::string& literal) {
    std::cout << "char: impossible" << std::endl;
    std::cout << "int: impossible" << std::endl;

    if (literal == "nanf" || literal == "nan") {
        std::cout << "float: nanf" << std::endl;
        std::cout << "double: nan" << std::endl;
    } else if (literal == "+inff" || literal == "+inf") {
        std::cout << "float: +inff" << std::endl;
        std::cout << "double: +inf" << std::endl;
    } else if (literal == "-inff" || literal == "-inf") {
        std::cout << "float: -inff" << std::endl;
        std::cout << "double: -inf" << std::endl;
    }
}

void ScalarConverter::convert(const std::string& literal) {
    if (isPseudoLiteral(literal))
        convertPseudoLiteral(literal);
    else if (isChar(literal))
        convertFromChar(literal);
    else if (isFloat(literal))
        convertFromFloat(literal);
    else if (isDouble(literal))
        convertFromDouble(literal);
    else if (isInt(literal))
        convertFromInt(literal);
    else {
        std::cout << "Error: Invalid literal format" << std::endl;
    }
}