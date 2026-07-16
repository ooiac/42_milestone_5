# CPP Module 09: STL in Practice

CPP Module 09 is the capstone of the C++98 track: three self-contained programs that put `std::map`, `std::stack`, and `std::vector`/`std::deque` to work on realistic problems instead of toy examples. There's no shared theme across exercises beyond "pick the STL container whose guarantees match the problem" — a sorted associative lookup, a LIFO expression evaluator, and a sorting algorithm compared across two container types.

## Overview

| Exercise | Class | Key Concepts |
|---|---|---|
| ex00 | `BitcoinExchange` | `std::map`, `upper_bound` for nearest-date lookup, robust input parsing |
| ex01 | `RPN` | `std::stack`, Reverse Polish Notation evaluation |
| ex02 | `PmergeMe` | Ford-Johnson merge-insertion sort, Jacobsthal sequence, `std::vector` vs `std::deque` |

**Important Constraint: C++98 Only**

Like every module in this track, this one is restricted to C++98:
- No `<algorithm>` conveniences beyond what's used here (`upper_bound`, `sort`, `min_element`/`max_element` from `<algorithm>` are fine — they're STL, not C++11)
- No `auto`, no range-based `for`, no lambdas
- Manual parsing (`strtof`, `atoi`) instead of `std::stof`/`std::stoi`

Compile with:
```makefile
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
```

---

## Exercise 00: BitcoinExchange

### Topics Covered
- `std::map<std::string, float>` as a sorted, searchable date → rate database
- `std::map::upper_bound` to find the closest date *at or before* a given date
- Defensive parsing: malformed dates, malformed values, out-of-range amounts

### Concept

`loadDatabase` reads `data.csv` (`date,exchange_rate` per line) into a `std::map`, which keeps entries sorted by key automatically — that ordering is what makes date lookups possible later:

```cpp
bool BitcoinExchange::loadDatabase(const std::string& filename) {
    // ...
    while (std::getline(file, line)) {
        // ...
        _database[date] = rate;
    }
    // ...
}
```

`processInput` reads the user's file (`date | value` per line), validates each line, and looks up the exchange rate for the *closest date not after* the requested one — `std::map::upper_bound` finds the first key strictly greater than the target, so stepping one entry back gives the last key ≤ target:

```cpp
std::map<std::string, float>::iterator it = _database.upper_bound(date);
if (it != _database.begin())
    --it;
else if (it == _database.begin() && _database.begin()->first > date)
{
    std::cerr << "Error: bad input => " << date << std::endl;
    continue;
}
```

Comparing dates as plain strings only works because `YYYY-MM-DD` sorts lexicographically identically to chronological order — no date-parsing library is needed for the comparison itself, only for validating the format (`isValidDate` checks length, dash positions, digit-only fields, and a year ≥ 2009). Every rejection prints the exact error the subject specifies (`bad input`, `not a positive number`, `too large a number`) and moves on to the next line rather than aborting.

### Compilation & Usage
```bash
cd ex00 && make
./btc input.txt
```
`data.csv` must be present in the working directory — `loadDatabase` reads it by that literal filename.

---

## Exercise 01: RPN (Reverse Polish Notation)

### Topics Covered
- `std::stack<int>` as the natural data structure for postfix evaluation
- Token-by-token parsing with `std::istringstream`
- Rejecting malformed expressions instead of guessing at intent

### Concept

RPN evaluation is a stack in its purest form: push every number, and on every operator, pop the top two operands, apply the operator, and push the result back:

```cpp
void RPN::performOperation(char op) {
    if (_stack.size() < 2)
        throw std::runtime_error("Error");

    int b = _stack.top(); _stack.pop();
    int a = _stack.top(); _stack.pop();
    // ... res = a <op> b ...
    _stack.push(res);
}
```

Order matters: the second-popped value (`a`) is the left operand and the first-popped (`b`) is the right, since the stack pops most-recently-pushed first. `evaluate` tokenizes the whole expression with `std::istringstream` and rejects anything that isn't a single-digit number or one of `+ - * /`:

```cpp
bool RPN::isNumber(const std::string &token) const {
    if (token.length() != 1)
        return false;
    return std::isdigit(static_cast<unsigned char>(token[0]));
}
```

The single-digit restriction is deliberate — the subject only requires digits 0-9 as operands, so multi-digit numbers and negative literals are correctly treated as invalid tokens rather than parsed. Division by zero throws the same generic `"Error"` as every other failure mode, and a valid expression must leave exactly one value on the stack at the end; anything else (too many operators, too many operands) is also an error.

### Compilation & Usage
```bash
cd ex01 && make
./RPN "8 9 * 9 - 9 - 9 - 4 - 1 +"
```

---

## Exercise 02: PmergeMe (Ford-Johnson Merge-Insertion Sort)

### Topics Covered
- The Ford-Johnson (merge-insertion) algorithm: pairing, recursively sorting the larger elements, then inserting the smaller elements via binary search in Jacobsthal order
- Implementing the same algorithm generically enough to run over both `std::vector` and `std::deque`
- Measuring and comparing container performance with `clock_gettime`

### Concept

Ford-Johnson beats a plain insertion sort by minimizing comparisons: elements are paired up, the larger of each pair is recursively sorted (the "main chain"), and only then are the smaller elements ("pend") inserted back — each via binary search, in an order given by the Jacobsthal sequence rather than left-to-right, which bounds the search range each insertion needs:

```cpp
std::vector<std::pair<int, int> > pairs;
// ... pair up arr[i], arr[i+1], larger first ...
for (size_t i = 0; i < pairs.size(); ++i) {
    mainChain.push_back(pairs[i].first);   // larger elements
    pend.push_back(pairs[i].second);        // smaller elements
}
fordJohnsonVector(mainChain);   // recurse on the larger half
```

The Jacobsthal sequence (`0, 1, 1, 3, 5, 11, 21, ...`, each term `J(n) = J(n-1) + 2*J(n-2)`) determines the *order* pend elements are inserted in, not their final position — inserting in this order keeps each binary search's range small, which is the whole point of the algorithm over naive insertion sort:

```cpp
size_t next = curr + 2 * prev;
```

The odd element left over when the array size is odd (the "straggler") is set aside before pairing and merged back in with one final binary-search insertion at the end. Below 17 elements, `fordJohnsonVector`/`fordJohnsonDeque` fall back to plain insertion sort — the overhead of pairing and Jacobsthal-ordered insertion isn't worth it at that size. The entire algorithm is implemented twice, once against `std::vector<int>` and once against `std::deque<int>`, so `sort()` can time both on the *same* input and let `main.cpp` report which container was faster:

```cpp
clock_gettime(CLOCK_MONOTONIC, &start);
fordJohnsonVector(_vectorData);
clock_gettime(CLOCK_MONOTONIC, &end);
_vectorTime = (end.tv_sec - start.tv_sec) * 1e6 + (end.tv_nsec - start.tv_nsec) / 1000.0;
```

Input is validated up front (`isValidNumber` rejects anything non-digit, including negative numbers) — a merge-insertion sort has no meaningful behavior on unparseable input, so bad arguments are rejected before any sorting begins.

### Compilation & Usage
```bash
cd ex02 && make
./PmergeMe 3 5 9 7 4
```

---

## Why Different STL Containers for Different Jobs

Each exercise picks its container for a specific structural reason: `std::map` because the problem is fundamentally "find the nearest key" over sorted associative data; `std::stack` because RPN evaluation *is* a stack machine; `std::vector`/`std::deque` because comparing their performance under identical insertion patterns is the exercise's actual point.

### Container Choice by Access Pattern

| Container | Access pattern needed | Used for |
|---|---|---|
| `std::map` | Sorted keys, nearest-match lookup (`upper_bound`) | Date → exchange-rate lookup (ex00) |
| `std::stack` | LIFO push/pop only | Postfix expression evaluation (ex01) |
| `std::vector` | Contiguous storage, random access, binary-searchable | Ford-Johnson sort, contiguous variant (ex02) |
| `std::deque` | Non-contiguous chunked storage, cheap front/back insertion | Ford-Johnson sort, chunked variant (ex02) |

`std::vector` insertion in the middle shifts every following element (O(n) per insert); `std::deque`'s chunked storage makes some insertions cheaper but loses raw cache locality — ex02 exists to make that trade-off measurable rather than theoretical.

---

## Learning Path

1. **ex00**: use `std::map`'s sorted-order guarantee and `upper_bound` to answer a "nearest key ≤ target" query without writing a search by hand.
2. **ex01**: use `std::stack` for exactly the job it names — a LIFO — to evaluate postfix notation in a single left-to-right pass.
3. **ex02**: implement a real algorithm (Ford-Johnson) against two different container types to see how the *same logic* performs differently depending on the underlying storage.

## Challenges

- **`upper_bound` off-by-one**: it returns the first key *greater* than the target, so the "nearest ≤" lookup needs one `--it` step, and the edge case where the target predates every database entry needs its own check.
- **Jacobsthal insertion order**: it's easy to insert pend elements left-to-right instead of in Jacobsthal order — the sort still produces correct output either way, so the bug only shows up as a missed performance requirement, not a wrong answer.
- **Single-digit-only RPN operands**: an implementation that "helpfully" parses multi-digit numbers or negative literals is stricter than the subject and actually less correct than one that rejects them.
- **Large inputs in PmergeMe**: `std::atoi` silently overflows on values ≥ `INT_MAX`; the subject's test data includes `2147483648` specifically to check this is rejected explicitly.

## Tips for Success

1. In ex00, validate the date string's *shape* (length, dash positions, digit-only fields) before ever calling `atoi` on substrings of it — a malformed date fed to `atoi` doesn't fail loudly.
2. In ex01, keep operand/operator token checks strict (single digit, exactly one of `+ - * /`) rather than permissive — the subject rewards rejecting malformed input over guessing at it.
3. In ex02, verify the algorithm's *comparison count* stays close to the theoretical Ford-Johnson bound, not just that the output ends up sorted — an accidentally-quadratic fallback path can still produce correct output while missing the point of the exercise.
4. Time both containers on the same generated input within the same run (not across separate runs) so the comparison isn't skewed by unrelated system noise.
5. Run under `valgrind --leak-check=full` — ex02 especially, since recursive pairing allocates many intermediate `std::vector`/`std::deque` instances.

## Resources

- [cppreference.com: std::map::upper_bound](https://en.cppreference.com/w/cpp/container/map/upper_bound)
- [cppreference.com: std::stack](https://en.cppreference.com/w/cpp/container/stack)
- [Wikipedia: Merge-insertion sort (Ford-Johnson algorithm)](https://en.wikipedia.org/wiki/Merge-insertion_sort)
- [cppreference.com: Jacobsthal number](https://en.wikipedia.org/wiki/Jacobsthal_number)
- [cppreference.com: std::upper_bound](https://en.cppreference.com/w/cpp/algorithm/upper_bound)

## Checklist for Completion

- [ ] `BitcoinExchange` loads `data.csv` and rejects a missing/unreadable file
- [ ] Invalid dates, negative values, and values > 1000 each print the exact specified error and continue to the next line
- [ ] The exchange rate used is the closest date **at or before** the requested date
- [ ] `RPN` correctly evaluates valid postfix expressions with `+ - * /`
- [ ] `RPN` rejects division by zero, malformed tokens, and leftover/insufficient stack values with `Error`
- [ ] `PmergeMe` sorts correctly for both `std::vector` and `std::deque` on the same input
- [ ] `PmergeMe` rejects negative numbers and non-numeric arguments
- [ ] `PmergeMe` prints before/after sequences and both containers' timings
- [ ] No memory leaks (`valgrind --leak-check=full` clean)
- [ ] Everything compiles cleanly with `-Wall -Wextra -Werror -std=c++98`

## Author

**caida-si**
*42*
