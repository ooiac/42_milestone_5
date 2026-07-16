# CPP Module 07: Templates

CPP Module 07 is the entry point into C++ templates: writing one function or class body that the compiler instantiates for whatever type it's called with, instead of hand-duplicating the same logic per type. Each exercise adds one layer — a plain function template, a template that takes a function pointer, then a template class with its own invariants.

## Overview

| Exercise | Function/Class | Key Concepts |
|---|---|---|
| ex00 | `::swap`, `::min`, `::max` | Function templates, template argument deduction |
| ex01 | `::iter` | Templates over a pointer + function pointer, overload resolution |
| ex02 | `Array<T>` | Class templates, deep copy, bounds-checked `operator[]` |

**Important Constraint: C++98 Only**

Like every module up to Module 07 (inclusive), this one is restricted to C++98:
- No `<algorithm>`, no STL containers (`vector`, `map`, ...)
- No `auto`, no range-based `for`, no lambdas
- Function pointers instead of `std::function`/lambdas for callbacks

Compile with:
```makefile
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98
```

---

## Exercise 00: whatever (swap, min, max)

### Topics Covered
- Function templates with a single type parameter
- Implicit template argument deduction from call-site arguments
- Writing one body that works identically for `int` and `std::string`

### Concept

`swap`, `min`, and `max` are declared once, generically, and instantiated per type at compile time — no overload has to be written by hand for `int` versus `std::string`:

```cpp
template <typename T>
void swap(T &a, T &b) {
    T tmp = a;
    a = b;
    b = tmp;
}

template <typename T>
T min(T a, T b) {
    return (a < b) ? a : b;
}
```

`min`/`max` take their arguments by value and return by value, so every call makes copies; that's fine for `int` but means `swap` (by reference) is the only one of the three cheap to call on a `std::string`. The compiler deduces `T` from the call site, so `::swap(a, b)` with two `int`s instantiates a different function than `::swap(c, d)` with two `std::string`s — the `::` prefix in `main.cpp` exists only to make sure the free template is called instead of a same-named member.

### Compilation & Usage
```bash
cd ex00 && make && ./whatever
```

---

## Exercise 01: iter

### Topics Covered
- A template function that takes both a generic array and a function pointer
- Overloading a template on the callback's constness (`void(&)` vs `void(const &)`)
- Applying an arbitrary operation across an array without knowing the element type

### Concept

`iter` walks a `T *array` of a given `length` and calls `func` on every element. Two overloads exist purely so `iter` can accept either a mutating callback or a read-only one:

```cpp
template <typename T>
void iter(T *array, const int length, void (*func)(T &)) {
    for (int i = 0; i < length; i++)
        func(array[i]);
}

template <typename T>
void iter(T *array, const int length, void (*func)(const T &)) {
    for (int i = 0; i < length; i++)
        func(array[i]);
}
```

`main.cpp` exercises both: `incrementElement`/`toUppercase` mutate in place through the first overload, `printElement` only reads through the second. Because `printElement` and `incrementElement` are themselves templates, `main` has to force instantiation explicitly at the call site (`printElement<int>`, `printElement<double>`, ...) since a bare function pointer doesn't give the compiler enough to deduce `T` from.

### Compilation & Usage
```bash
cd ex01 && make && ./iter
```

---

## Exercise 02: Array

### Topics Covered
- A full class template with the canonical form (constructor, copy constructor, `operator=`, destructor)
- Deep-copy semantics for a heap-allocated buffer
- A bounds-checked `operator[]` that throws instead of invoking undefined behavior

### Concept

`Array<T>` owns a `T *_data` allocated with `new T[n]()`. The empty parentheses matter: they value-initialize every element (`0` for `int`, empty string for `std::string`), so a freshly constructed `Array<int>` never holds garbage:

```cpp
Array(unsigned int n) : _size(n) {
    _data = new T[n]();
}
```

Copy construction and `operator=` both allocate a fresh buffer and copy element by element rather than sharing the pointer, so two `Array` objects never alias the same storage:

```cpp
Array(const Array &other) : _size(other._size) {
    _data = new T[_size];
    for (unsigned int i = 0; i < _size; i++)
        _data[i] = other._data[i];
}
```

`operator[]` is bounds-checked in both its `const` and non-`const` overloads. Because `_size` is `unsigned int`, a negative index passed as `int` wraps around to a huge unsigned value — which still correctly fails the `index >= _size` check and throws rather than reading out of bounds:

```cpp
T &operator[](unsigned int index) {
    if (index >= _size)
        throw std::out_of_range("Index out of bounds");
    return _data[index];
}
```

`main.cpp` builds a 750-element `Array<int>`, copies it through a temporary scope, and confirms the original is untouched by the copy — proving the deep copy actually deep-copies rather than sharing `_data`.

### Compilation & Usage
```bash
cd ex02 && make && ./array
```

---

## Why Templates, Not Copy-Paste or Macros

A macro-based "generic" `SWAP(a, b)` is textually substituted before the compiler ever sees a type, so it has no type checking and can silently misbehave (double-evaluating an argument with side effects, for instance). Hand-duplicating `swapInt`, `swapString`, `swapFloat` works but means every bug fix has to be repeated N times. A template gets the compiler to generate exactly the specialized code each call site needs, with full type checking, at compile time — one definition, N type-safe instantiations.

### Templates vs. Alternatives

| Aspect | Templates | Macros | Manual overloads |
|---|---|---|---|
| Type-checked | Yes, at instantiation | No (pure text substitution) | Yes |
| Code duplication in source | None (one definition) | None (but no safety) | One copy per type |
| Debuggable | Yes (real functions in the debugger) | No (expanded before compilation) | Yes |
| Works with any type meeting the requirements | Yes | Yes (unsafely) | No (only types written by hand) |

---

## Learning Path

1. **ex00**: get comfortable with `template <typename T>` on the simplest possible functions, and see the compiler deduce `T` from arguments.
2. **ex01**: pass a function pointer alongside a generic array, and see why two overloads are needed to support both mutating and read-only callbacks.
3. **ex02**: wrap the same genericity around a whole class with ownership semantics — a class template needs a correct copy constructor and `operator=`, not just one function body.

## Challenges

- **Explicit instantiation for function-pointer arguments**: `iter(intArray, len, printElement<int>)` needs the `<int>` because a plain function pointer can't be used to deduce a template parameter the way a value argument can.
- **`new T[n]()` vs `new T[n]`**: dropping the parentheses leaves primitive-type elements uninitialized, which only shows up as a bug once `operator[]` is read before being written.
- **Unsigned index wraparound**: `operator[](-1)` on an `unsigned int` parameter doesn't crash silently — verifying it converts to a huge value and still gets caught by the bounds check is worth testing explicitly.

## Tips for Success

1. Keep template definitions entirely in the header — with no explicit instantiation, the compiler needs the full body visible at every call site, which is why `Array.hpp` has no matching `.cpp`.
2. Write the class template's copy constructor and `operator=` to allocate before copying and free the old buffer in `operator=` (self-assignment check first), exactly as for a non-template class.
3. Test both `operator[]` overloads (`const` and non-`const`) — it's easy to write only one and have the other silently fall back to something unintended.
4. Run under `valgrind --leak-check=full` on ex02; a missing `delete[]` in the destructor or a leaked temporary during copy-assignment is the most common bug.

## Resources

- [cppreference.com: Function templates](https://en.cppreference.com/w/cpp/language/function_template)
- [cppreference.com: Class templates](https://en.cppreference.com/w/cpp/language/class_template)
- [cppreference.com: Template argument deduction](https://en.cppreference.com/w/cpp/language/template_argument_deduction)
- "Effective C++" by Scott Meyers (items on templates and generic programming)

## Checklist for Completion

- [ ] `swap`/`min`/`max` work identically for `int` and `std::string`
- [ ] `iter` supports both a mutating and a read-only function pointer via overloading
- [ ] `Array<T>` default-constructs to size 0 with a `NULL` buffer
- [ ] `Array<T>` value-initializes elements when constructed with a size
- [ ] Copy constructor and `operator=` perform a true deep copy (no shared `_data`)
- [ ] `operator[]` throws `std::out_of_range` on an invalid index instead of reading/writing out of bounds
- [ ] No memory leaks (`valgrind --leak-check=full` clean)
- [ ] Everything compiles cleanly with `-Wall -Wextra -Werror -std=c++98`

## Author

**caida-si**
*42*
