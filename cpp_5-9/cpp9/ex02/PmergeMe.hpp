//
// Created by caida-si on 16/07/26.
//

#ifndef PMERGEME_HPP
#define PMERGEME_HPP

#include <vector>
#include <deque>
#include <string>
#include <iostream>

class PmergeMe {
private:
    std::vector<int> _vectorData;
    std::deque<int> _dequeData;
    std::vector<int> _originalData;
    double _vectorTime;
    double _dequeTime;

// Jacobsthal sequence
	size_t jacobsthal(size_t n);
	std::vector<size_t> generateJacobsthalSequence(size_t len);

// Vector sorting
    void fordJohnsonVector(std::vector<int>& arr);
	void insertPendingVector(std::vector<int>& mainChain, std::vector<int>& pend, const std::vector<size_t>& jacobSeq);

// Deque sorting
	void fordJohnsonDeque(std::deque<int>& arr);
	void insertPendingDeque(std::deque<int>& mainChain, std::deque<int>& pend, const std::deque<size_t>& jacobSeq);

	bool isValidNumber(const std::string& str) const;

public:
    PmergeMe();
    PmergeMe(const PmergeMe& other);
    PmergeMe& operator=(const PmergeMe& other);
    ~PmergeMe();

    bool parseInput(int argc, char** argv);
    void sort();
    void displayResults() const;
};

#endif // PMERGEME_HPP