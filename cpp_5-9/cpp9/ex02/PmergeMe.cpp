//
// Created by caida-si on 16/07/26.
//

#include "PmergeMe.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <ctime>
#include <set>

PmergeMe::PmergeMe() : _vectorTime(0.0), _dequeTime(0.0) {}

PmergeMe::PmergeMe(const PmergeMe& other)
    : _vectorData(other._vectorData),
      _dequeData(other._dequeData),
      _originalData(other._originalData),
      _vectorTime(other._vectorTime),
      _dequeTime(other._dequeTime) {}

PmergeMe& PmergeMe::operator=(const PmergeMe& other) {
    if (this != &other) {
        _vectorData = other._vectorData;
        _dequeData = other._dequeData;
        _originalData = other._originalData;
        _vectorTime = other._vectorTime;
        _dequeTime = other._dequeTime;
    }
    return *this;
}

PmergeMe::~PmergeMe() {}

// Jacobsthal sequence and its generation
size_t PmergeMe::jacobsthal(size_t n) {
    if (n == 0) return 0;
    if (n == 1) return 1;

    size_t prev = 0;
    size_t curr = 1;

    for (size_t i = 2; i <= n; ++i) {
        size_t next = curr + 2 * prev;
        prev = curr;
        curr = next;
    }
    return curr;
}

std::vector<size_t> PmergeMe::generateJacobsthalSequence(size_t len) {
    std::vector<size_t> sequence;
    if (len == 0) return sequence;

    size_t jacobIdx = 3;
    size_t prevJacob = jacobsthal(jacobIdx - 1);
    size_t currJacob = jacobsthal(jacobIdx);

    while (prevJacob < len) {
        size_t start = (currJacob < len) ? prevJacob : len;
        for (size_t i = start; i > prevJacob; --i) {
            sequence.push_back(i - 1);
        }

        jacobIdx++;
        prevJacob = currJacob;
        currJacob = jacobsthal(jacobIdx);
    }
    return sequence;
}

bool PmergeMe::isValidNumber(const std::string& str) const {
    if (str.empty() || str[0] == '-') return false;
    for (size_t i = 0; i < str.length(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(str[i])))
            return false;
    }
    return true;
}

bool PmergeMe::parseInput(int ac, char** av) {
    if (ac < 2) return false;

    for (int i = 1; i < ac; ++i)
    {
        if (!isValidNumber(av[i]))
            return false;

        int num = std::atoi(av[i]);
        _vectorData.push_back(num);
        _dequeData.push_back(num);
    }

    _originalData = _vectorData;
    return true;
}

// Ford-Johnson sorting (with Jacobsthal sequence) with std::vector
void PmergeMe::insertPendingVector(std::vector<int>& mainChain, std::vector<int>& pend, const std::vector<size_t>& jacobSeq) {
    if (pend.empty()) return;

    std::set<size_t> inserted;

    for (size_t i = 0; i < jacobSeq.size() && jacobSeq[i] < pend.size(); ++i) {
        size_t idx = jacobSeq[i];
        int value = pend[idx];

        std::vector<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), value);
        mainChain.insert(pos, value);
        inserted.insert(idx);
    }

    for (size_t i = 0; i < pend.size(); ++i) {
        if (inserted.find(i) == inserted.end()) {
            std::vector<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), pend[i]);
            mainChain.insert(pos, pend[i]);
        }
    }
}

void PmergeMe::fordJohnsonVector(std::vector<int>& arr) {
    if (arr.size() <= 1) return;

    if (arr.size() <= 16) {
        for (size_t i = 1; i < arr.size(); ++i) {
            int key = arr[i];
            int j = static_cast<int>(i) - 1;
            while (j >= 0 && arr[j] > key) {
                arr[j + 1] = arr[j];
                --j;
            }
            arr[j + 1] = key;
        }
        return;
    }

    std::vector<std::pair<int, int> > pairs;
    bool hasStraggler = (arr.size() % 2 != 0);
    int straggler = hasStraggler ? arr[arr.size() - 1] : 0;
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        if (arr[i] > arr[i + 1])
            pairs.push_back(std::make_pair(arr[i], arr[i + 1]));
        else
            pairs.push_back(std::make_pair(arr[i + 1], arr[i]));
    }

    std::vector<int> mainChain;
    std::vector<int> pend;
    for (size_t i = 0; i < pairs.size(); ++i) {
        mainChain.push_back(pairs[i].first);
        pend.push_back(pairs[i].second);
    }

    fordJohnsonVector(mainChain);

    if (!pend.empty()) {
        mainChain.insert(mainChain.begin(), pend[0]);
        pend.erase(pend.begin());
    }

    std::vector<size_t> jacobSeq = generateJacobsthalSequence(pend.size());
    insertPendingVector(mainChain, pend, jacobSeq);

    if (hasStraggler) {
        std::vector<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), straggler);
        mainChain.insert(pos, straggler);
    }

    arr = mainChain;
}

// Ford-Johnson sorting (with Jacobsthal sequence) with std::deque
void PmergeMe::insertPendingDeque(std::deque<int>& mainChain, std::deque<int>& pend, const std::deque<size_t>& jacobSeq) {
    if (pend.empty()) return;

    std::set<size_t> inserted;

    for (size_t i = 0; i < jacobSeq.size() && jacobSeq[i] < pend.size(); ++i) {
        size_t idx = jacobSeq[i];
        int value = pend[idx];

        std::deque<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), value);
        mainChain.insert(pos, value);
        inserted.insert(idx);
    }

    for (size_t i = 0; i < pend.size(); ++i) {
        if (inserted.find(i) == inserted.end()) {
            std::deque<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), pend[i]);
            mainChain.insert(pos, pend[i]);
        }
    }
}

void PmergeMe::fordJohnsonDeque(std::deque<int>& arr) {
    if (arr.size() <= 1) return;

    if (arr.size() <= 16) {
        for (size_t i = 1; i < arr.size(); ++i) {
            int key = arr[i];
            int j = static_cast<int>(i) - 1;
            while (j >= 0 && arr[j] > key) {
                arr[j + 1] = arr[j];
                --j;
            }
            arr[j + 1] = key;
        }
        return;
    }

    std::vector<std::pair<int, int> > pairs;
    bool hasStraggler = (arr.size() % 2 != 0);
    int straggler = hasStraggler ? arr[arr.size() - 1] : 0;
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        if (arr[i] > arr[i + 1])
            pairs.push_back(std::make_pair(arr[i], arr[i + 1]));
        else
            pairs.push_back(std::make_pair(arr[i + 1], arr[i]));
    }

    std::deque<int> mainChain;
    std::deque<int> pend;
    for (size_t i = 0; i < pairs.size(); ++i) {
        mainChain.push_back(pairs[i].first);
        pend.push_back(pairs[i].second);
    }

    fordJohnsonDeque(mainChain);

    if (!pend.empty()) {
        mainChain.insert(mainChain.begin(), pend[0]);
        pend.erase(pend.begin());
    }

    std::vector<size_t> jacobSeqVec = generateJacobsthalSequence(pend.size());
    std::deque<size_t> jacobSeq(jacobSeqVec.begin(), jacobSeqVec.end());
    insertPendingDeque(mainChain, pend, jacobSeq);

    if (hasStraggler) {
        std::deque<int>::iterator pos = std::upper_bound(mainChain.begin(), mainChain.end(), straggler);
        mainChain.insert(pos, straggler);
    }

    arr = mainChain;
}

void PmergeMe::sort()
{
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    fordJohnsonVector(_vectorData);
    clock_gettime(CLOCK_MONOTONIC, &end);
    _vectorTime = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1000.0;

    clock_gettime(CLOCK_MONOTONIC, &start);
    fordJohnsonDeque(_dequeData);
    clock_gettime(CLOCK_MONOTONIC, &end);
    _dequeTime = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1000.0;
}

void PmergeMe::displayResults() const {
    std::cout << "Before: ";
    for (size_t i = 0; i < _originalData.size() && i < 5; ++i) {
        std::cout << _originalData[i];
        if (i < 4 && i + 1 < _originalData.size()) {
            std::cout << " ";
        }
    }
    if (_originalData.size() > 5) {
        std::cout << " [...]";
    }
    std::cout << std::endl;

    std::cout << "After:  ";
    for (size_t i = 0; i < _vectorData.size() && i < 5; ++i) {
        std::cout << _vectorData[i];
        if (i < 4 && i + 1 < _vectorData.size()) {
            std::cout << " ";
        }
    }
    if (_vectorData.size() > 5) {
        std::cout << " [...]";
    }
    std::cout << std::endl;

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Time to process a range of " << _vectorData.size()
              << " elements with std::vector : " << _vectorTime << " µs" << std::endl;
    std::cout << "Time to process a range of " << _dequeData.size()
              << " elements with std::deque  : " << _dequeTime << " µs" << std::endl;
}