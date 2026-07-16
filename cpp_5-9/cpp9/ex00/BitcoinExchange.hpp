//
// Created by caida-si on 16/07/26.
//

#ifndef BITCOINEXCHANGE_HPP
#define BITCOINEXCHANGE_HPP

#include <string>
#include <map>
#include <iostream>

class BitcoinExchange {
private:
    std::map<std::string, float> _database;

    bool isValidDate(const std::string& date) const;
    bool isValidValue(const std::string& value) const;
    float stringToFloat(const std::string& str) const;

public:
    BitcoinExchange();
    BitcoinExchange(const BitcoinExchange& src);
    BitcoinExchange& operator=(const BitcoinExchange& rhs);
    ~BitcoinExchange();

    bool loadDatabase(const std::string& filename);
    void processInput(const std::string& filename);
};

#endif //BITCOINEXCHANGE_HPP