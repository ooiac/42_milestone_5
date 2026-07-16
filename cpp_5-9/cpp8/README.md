# CPP Module 08: Templated Containers, Iterators, Algorithms

CPP Module 08 opens up the STL after seven modules of writing everything by hand. The theme is genericity at three levels at once: a function generic over *any* container (ex00), a class that's a thin, type-safe wrapper around `std::vector` (ex01), and a class that inherits STL container behavior it wasn't originally given (ex02).

## Overview

| Exercise | Class/Function | Key Concepts |
|---|---|---|
| ex00 | `easyfind` | Generic algorithms over iterators, `std::find`, `typename` for dependent types |
| ex01 | `Span` | Wrapping `std::vector`, `std::sort`, `std::min_element`/`std::max_element`, iterator-range constructors |
| ex02 | `MutantStack<T>` | Inheriting from an STL container adaptor, exposing its hidden iterators |

**Important Constraint: C++98 Only**

Like every module up to Module 08, this one is restricted to C++98:
- STL containers and `<algorithm>` are finally allowed (that's the point of the module)
- Still no `auto`, no range-based `for`, no lambdas — iterators are spelled out by hand
- No C++11 container methods (`cbegin`/`cend`, initializer-list constructors, etc.)

Compile with:
```makefile
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
```

---

## Exercise 00: easyfind

### Topics Covered
- A function template generic over the *container type*, not just the element type
- `std::find` from `<algorithm>`
- `typename T::iterator` — telling the compiler that `T::iterator` names a type, not a value

### Concept

`easyfind` works on any container `T` that exposes `begin()`, `end()`, and an `iterator` typedef — it never mentions `std::vector` or `std::list` by name:

```cpp
template <typename T>
typename T::iterator easyfind(T &container, int value) {
    typename T::iterator it = std::find(container.begin(), container.end(), value);
    if (it == container.end())
        throw std::runtime_error("Value not found in container");
    return it;
}
```

The `typename` keyword before `T::iterator` is required, not decorative: since `T` is a template parameter, the compiler can't know until instantiation whether `T::iterator` names a type or a static member value, and `typename` resolves that ambiguity. `main.cpp` instantiates `easyfind` against both `std::vector<int>` and `std::list<int>` — the same function body, unmodified, works for a container with random-access iterators and one with only bidirectional iterators, because `std::find` only ever needs `==` and `++`.

### Compilation & Usage
```bash
cd ex00 && make && ./easyfind
```

---

## Exercise 01: Span

### Topics Covered
- A class that owns a fixed-capacity `std::vector<int>` internally
- `std::sort`, `std::min_element`, `std::max_element`
- A member function template (`addNumbers`) accepting any `InputIterator` range

### Concept

`Span` is constructed with a maximum capacity `N` and rejects any attempt to store more than `N` numbers:

```cpp
void Span::addNumber(int number) {
    if (_numbers.size() >= _maxSize)
        throw std::out_of_range("Span is full");
    _numbers.push_back(number);
}
```

`shortestSpan()` sorts a *copy* of the stored numbers (never disturbing the original order) and scans adjacent pairs for the minimum gap — after sorting, the smallest difference between any two elements is guaranteed to be between neighbors:

```cpp
int Span::shortestSpan(void) const {
    std::vector<int> sorted = _numbers;
    std::sort(sorted.begin(), sorted.end());

    int shortest = INT_MAX;
    for (size_t i = 0; i < sorted.size() - 1; ++i) {
        int diff = sorted[i + 1] - sorted[i];
        if (diff < shortest)
            shortest = diff;
    }
    return shortest;
}
```

`longestSpan()` doesn't need a full sort at all — `std::min_element`/`std::max_element` find the extremes in one linear pass each, and their difference is the widest span by definition. `addNumbers` is a member function *template* over `InputIterator`, so it accepts a range from any container (`main.cpp` feeds it a `std::vector<int>::iterator` pair) without `Span` ever needing to know the source container's type:

```cpp
template <typename InputIterator>
void addNumbers(InputIterator begin, InputIterator end) {
    for (InputIterator it = begin; it != end; ++it) {
        if (_numbers.size() >= _maxSize)
            throw std::out_of_range("Span is full");
        _numbers.push_back(*it);
    }
}
```

Both span functions throw `std::logic_error` if fewer than two numbers have been added — a span isn't defined for 0 or 1 elements. `main.cpp` also stress-tests `Span` at 10,000 elements to confirm the sort-based approach stays fast.

### Compilation & Usage
```bash
cd ex01 && make && ./span
```

---

## Exercise 02: MutantStack

### Topics Covered
- Public inheritance from `std::stack<T>` to add functionality the base class doesn't expose
- Reaching into `std::stack`'s protected `c` member to iterate its underlying container
- `iterator`/`const_iterator`/`reverse_iterator`/`const_reverse_iterator` typedefs pulled from the adaptor's `container_type`

### Concept

`std::stack` is deliberately not iterable — it only exposes `push`/`pop`/`top`. `MutantStack<T>` inherits from it and adds iteration by reaching into the protected `c` member (the underlying container, `std::deque<T>` by default) that `std::stack` keeps hidden from ordinary users but visible to subclasses:

```cpp
template <typename T>
class MutantStack : public std::stack<T> {
public:
    typedef typename std::stack<T>::container_type::iterator iterator;
    // ...
    iterator begin() { return this->c.begin(); }
    iterator end() { return this->c.end(); }
    // ...
};
```

Everything else — `push`, `pop`, `top`, `size`, the copy constructor and `operator=` — is inherited from `std::stack` unchanged; `MutantStack` adds exactly the four iterator-returning pairs and nothing else. `main.cpp` demonstrates the result is a stack that behaves like `std::stack` for LIFO access but can also be walked front-to-back or back-to-front like a `std::list`, and that it converts cleanly to a plain `std::stack<int>` via the inherited copy constructor.

### Compilation & Usage
```bash
cd ex02 && make && ./MutantStack
```

---

## Why the STL, Not Hand-Rolled Containers

Modules 00-07 build everything — dynamic arrays, linked structures, sorting — from scratch to force understanding of what a container or algorithm actually does under the hood. Module 08 is the payoff: `std::vector`, `std::list`, `std::stack`, and `<algorithm>` are battle-tested, allocate correctly, and compose with each other through the shared iterator interface, so `std::find` and `std::sort` work identically whether they're handed a `vector` or a hand-inherited `stack`'s guts.

### Generic Algorithms vs. Hand-Written Loops

| Aspect | Generic algorithm (`std::find`, `std::sort`) | Hand-written loop per container |
|---|---|---|
| Works across container types | Yes (anything with matching iterators) | No (rewritten per container) |
| Correctness | Tested by the standard library | Reimplemented, re-bugged each time |
| Complexity guarantees | Documented (`std::sort` is O(n log n)) | Whatever the hand-written version happens to be |
| Expresses intent | Yes (`std::find` reads as "find") | Buried in loop/index bookkeeping |

---

## Learning Path

1. **ex00**: write a function generic over the *container*, using only the iterator interface (`begin`/`end`/`==`/`++`) that every STL container shares.
2. **ex01**: wrap a `std::vector` inside a class with its own invariant (capacity), and use `std::sort`/`std::min_element`/`std::max_element` instead of hand-rolled scans.
3. **ex02**: inherit from an STL adaptor to add capability it deliberately withholds, using the protected access inheritance grants that composition wouldn't.

## Challenges

- **`typename` before a dependent type**: forgetting it on `T::iterator` inside a template is a compile error that doesn't explain itself well the first time it's hit.
- **Sorting a copy, not the original, in `shortestSpan`**: mutating `_numbers` in place would silently break `addNumber`'s ordering guarantees for any caller that added more numbers afterward.
- **Reaching into `std::stack`'s `c`**: it's `protected`, not `public`, so it's only reachable through inheritance — the exercise exists specifically to make that distinction concrete.
- **10,000-element stress test in ex01**: makes an accidentally-quadratic implementation (e.g., re-sorting inside a loop) visible immediately instead of only at scale.

## Tips for Success

1. In ex00, resist adding a container-specific special case — if the implementation compiles with both `std::vector` and `std::list` unchanged, it's generic enough.
2. In ex01, throw `std::logic_error` (not `std::out_of_range`) for the "fewer than two elements" case — they're different failure conditions and the subject expects them distinguished.
3. In ex02, add nothing beyond the four iterator accessors; `MutantStack` should differ from `std::stack` in exactly one way.
4. Run under `valgrind --leak-check=full` — `Span` and `MutantStack` both own STL containers that manage their own memory, so a leak here usually means an exception path skipped a destructor, not a missing `delete`.

## Resources

- [cppreference.com: std::find](https://en.cppreference.com/w/cpp/algorithm/find)
- [cppreference.com: std::stack](https://en.cppreference.com/w/cpp/container/stack)
- [cppreference.com: std::sort](https://en.cppreference.com/w/cpp/algorithm/sort)
- [cppreference.com: Iterator concepts](https://en.cppreference.com/w/cpp/iterator)
- "Effective STL" by Scott Meyers

## Checklist for Completion

- [ ] `easyfind` compiles and works unmodified against both `std::vector<int>` and `std::list<int>`
- [ ] `easyfind` throws when the value isn't found instead of returning a sentinel
- [ ] `Span::addNumber` throws `std::out_of_range` once capacity is reached
- [ ] `Span::addNumbers` accepts an iterator range from any container
- [ ] `shortestSpan`/`longestSpan` throw `std::logic_error` with fewer than 2 elements
- [ ] `Span` handles 10,000 elements without excessive runtime
- [ ] `MutantStack` supports `begin`/`end`/`rbegin`/`rend` while keeping all `std::stack` behavior
- [ ] No memory leaks (`valgrind --leak-check=full` clean)
- [ ] Everything compiles cleanly with `-Wall -Wextra -Werror -std=c++98`

## Author

**caida-si**
*42*
