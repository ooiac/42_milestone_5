//
// Created by caida-si on 16/07/26.
//

#ifndef RPN_HPP
#define RPN_HPP

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <stack>
#include <string>

class RPN {
private:
	std::stack<int> _stack;

	bool isOperator(const std::string &token) const;
	bool isNumber(const std::string &token) const;
	void performOperation(char op);

public:
	RPN();
	RPN(const RPN &src);
	RPN &operator=(const RPN &rhs);
	~RPN();

	bool evaluate(const std::string &expression);
	int getResult() const;
};

#endif //RPN_HPP