//
// Created by caida-si on 16/07/26.
//

#include "RPN.hpp"

RPN::RPN() {}

RPN::RPN(const RPN &src) { _stack = src._stack; }

RPN &RPN::operator=(const RPN &rhs) {
	if (this != &rhs) {
		while (!_stack.empty()) {
			_stack.pop();
		}
		_stack = rhs._stack;
	}
	return *this;
}

RPN::~RPN() {
	while (!_stack.empty())
		_stack.pop();
}

bool RPN::isOperator(const std::string &token) const {
	return token == "+" || token == "-" || token == "*" || token == "/";
}

bool RPN::isNumber(const std::string &token) const
{
	if (token.length() != 1)
		return false;
	return std::isdigit(static_cast<unsigned char>(token[0]));
}

void RPN::performOperation(char op) {
	if (_stack.size() < 2)
		throw std::runtime_error("Error");

	int b = _stack.top(); _stack.pop();
	int a = _stack.top(); _stack.pop();
	int res = 0;

	switch (op)
	{
	case '+':
		res = a + b;
		break;
	case '-':
		res = a - b;
		break;
	case '*':
		res = a * b;
		break;
	case '/':
		if (b == 0)
			throw std::runtime_error("Error");
		res = a / b;
		break;
	default:
		throw std::runtime_error("Error");
	}
	_stack.push(res);
}

bool RPN::evaluate(const std::string &expression) {
	std::istringstream iss(expression);
	std::string token;

	try	{
		while (iss >> token) {
			if (isNumber(token)) {
				_stack.push(std::atoi(token.c_str()));
			} else if (isOperator(token)) {
				performOperation(token[0]);
			} else {
				return false;
			}
		}

		if (_stack.size() != 1)
			return false;
		return true;
	} catch (const std::exception &e) {
		return false;
	}
}

int RPN::getResult() const {
	if (_stack.empty())
		return 0;
	return _stack.top();
}