//
// Created by caida-si on 16/07/26.
//

#include "BitcoinExchange.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>

BitcoinExchange::BitcoinExchange() {}

BitcoinExchange::BitcoinExchange(const BitcoinExchange& src) : _database(src._database) {}

BitcoinExchange& BitcoinExchange::operator=(const BitcoinExchange& rhs) {
    if (this != &rhs)
        _database = rhs._database;
    return *this;
}

BitcoinExchange::~BitcoinExchange() {}

bool BitcoinExchange::isValidDate(const std::string& date) const {
    if (date.length() != 10 || date[4] != '-' || date[7] != '-')
        return false;

    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit(date[i]))
            return false;
    }

    int year = std::atoi(date.substr(0, 4).c_str());
    int month = std::atoi(date.substr(5, 2).c_str());
    int day = std::atoi(date.substr(8, 2).c_str());

    if (month < 1 || month > 12 || day < 1 || day > 31)
        return false;
    if (year < 2009)
        return false;

    return true;
}

bool BitcoinExchange::isValidValue(const std::string& value) const {
    if (value.empty())
        return false;

    float val = stringToFloat(value);
    return val >= 0 && val <= 1000;
}

float BitcoinExchange::stringToFloat(const std::string& value) const {
    char *end = NULL;
    float result = strtof(value.c_str(), &end);

    if (end == value.c_str() || *end != '\0')
        return -1;
    return result;
}

bool BitcoinExchange::loadDatabase(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        return false;

    std::string line;
    std::getline(file, line); //to skip header

    while (std::getline(file ,line)) {
        size_t pos = line.find(',');
        if (pos == std::string::npos) continue;

        std::string date = line.substr(0, pos);
        std::string rateStr = line.substr(pos + 1);

        float rate = stringToFloat(rateStr);
        if (rate < 0) continue;
        _database[date] = rate;
    }
    file.close();
    return !_database.empty();
}

void BitcoinExchange::processInput(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Error: could not open file." << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); //to skip header

    while (std::getline(file ,line)) {
        size_t pos = line.find('|');
        if (pos == std::string::npos){
            if (line.find('-') != std::string::npos)
                std::cerr << "Error: bad input => " << line << std::endl;
            continue;
        }

        std::string date = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        while (!date.empty() && isspace(date[date.length() - 1])) //trim whitespace
            date.erase(date.length() - 1);
        while (!value.empty() && isspace(value[0]))
            value.erase(0, 1);
        while (!value.empty() && isspace(value[value.length() - 1]))
            value.erase(value.length() - 1);

        if (!isValidDate(date)) {
            std::cerr << "Error: bad input => " << date << std::endl;
            continue;
        }

        float amount = stringToFloat(value);
        if (amount < 0) {
            std::cerr << "Error: not a positive number." << std::endl;
            continue;
        }
        if (amount > 1000) {
            std::cerr << "Error: too large a number." << std::endl;
            continue;
        }

        std::map<std::string, float>::iterator it = _database.upper_bound(date); // find closest date (lower or equal)
        if (it != _database.begin())
            --it;
        else if (it == _database.begin() && _database.begin()->first > date)
        {
            std::cerr << "Error: bad input => " << date << std::endl;
            continue;
        }

        float rate = it->second;
        float result = amount * rate;

        std::cout << date << " => " << amount << " = ";
        std::cout << std::fixed << std::setprecision(2) << result << std::endl;
    }
    file.close();
}
