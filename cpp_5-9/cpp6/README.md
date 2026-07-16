# CPP Module 06: Type Conversions

CPP Module 06 is about C++'s cast operators and what they actually promise. Each exercise removes a different escape hatch you'd normally reach for in C: no `sscanf`/`sprintf` for parsing, no `void*` for storage, no `if`/`else if` chains for runtime type checks. Instead: `static_cast`, `reinterpret_cast`, and `dynamic_cast`, each used for exactly the job it's suited to.

## Overview

| Exercise | Class(es) | Key Concepts |
|---|---|---|
| ex00 | `ScalarConverter` | `static_cast`, literal parsing, edge cases (NaN/inf/overflow) |
| ex01 | `Serializer`, `Data` | `reinterpret_cast`, `uintptr_t` round-tripping |
| ex02 | `Base`, `A`, `B`, `C` | `dynamic_cast`, RTTI, pointer vs. reference identification |

**Important Constraint: C++98 Only**

Like every module up to Module 07, this one is restricted to C++98:
- No `<algorithm>`, no STL containers (`vector`, `map`, ...)
- No `auto`, no range-based `for`, no lambdas
- Manual literal parsing (`strtol`/`atof`), no `std::stoi`/`std::stod`

Compile with:
```makefile
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
```

---

## Exercise 00: ScalarConverter

### Topics Covered
- `static_cast` between `char`, `int`, `float`, and `double`
- Detecting a literal's type from its textual form alone
- Handling values that can't survive the conversion (overflow, NaN, infinity)

### Concept

`ScalarConverter` is a non-instantiable utility class: constructor, copy constructor, `operator=`, and destructor are all private. Its single public entry point, `convert()`, inspects the input string and dispatches to the matching parser:

```cpp
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
    else
        std::cout << "Error: Invalid literal format" << std::endl;
}
```

Every `is*` check is a hand-rolled scan over the string, not a call into `<sstream>` or `strtod`. `isFloat` requires a trailing `f` and a decimal point; `isDouble` requires the decimal point but no `f`; `isInt` requires digits only, no dot. Pseudo-literals (`nan`, `nanf`, `+inf`, `-inf`, `+inff`, `-inff`) are checked first since they don't parse as any of the others.

Each `convertFrom*` function prints all four representations, and each one independently decides whether a given target type is reachable:

```cpp
void ScalarConverter::convertFromInt(const std::string& literal) {
    errno = 0;
    char* end;
    long value = strtol(literal.c_str(), &end, 10);

    if (errno == ERANGE || value > INT_MAX || value < INT_MIN) {
        std::cout << "char: impossible" << std::endl;
        // ... int/float/double: impossible
        return;
    }
    // ...
}
```

`char` output has three outcomes, not two: a printable char (`'a'`), `Non displayable` for values 0-31/127+ that are valid `char`s but unprintable, and `impossible` for anything out of `char` range entirely (including NaN/inf and float/double values outside 0-127).

### Compilation & Usage
```bash
cd ex00 && make
./convert 42
./convert 42.0f
./convert 'c'
./convert nan
./convert 42.
```

---

## Exercise 01: Serializer

### Topics Covered
- `reinterpret_cast`, the cast that reinterprets bits without changing them
- `uintptr_t`, an integer type guaranteed wide enough to hold a pointer
- Why a round trip through an integer is safe here but not in general

### Concept

`Serializer` is another non-instantiable class with exactly two static functions, and both are one-liners:

```cpp
uintptr_t Serializer::serialize(Data* ptr) {
    return reinterpret_cast<uintptr_t>(ptr);
}

Data* Serializer::deserialize(uintptr_t raw) {
    return reinterpret_cast<Data*>(raw);
}
```

`Data` is a plain aggregate (`int id`, `std::string name`, `double value`) with no behavior of its own — it exists purely as a payload to serialize. `main.cpp` takes the address of a stack `Data`, serializes it to an integer, deserializes that integer back to a pointer, and confirms it's the exact same address:

```cpp
uintptr_t serialized = Serializer::serialize(&originalData);
Data* deserialized = Serializer::deserialize(serialized);

if (deserialized == &originalData)
    std::cout << "Pointers match! Serialization successful." << std::endl;
```

`static_cast` refuses this conversion outright — a pointer and an integer aren't related types. `reinterpret_cast` is the tool that says "treat these bits as something else," which is exactly what's needed and exactly why it's dangerous in less controlled contexts: it does zero validation, so misusing it just corrupts memory silently instead of raising a compile error.

### Compilation & Usage
```bash
cd ex01 && make && ./serialize
```

---

## Exercise 02: Base, A, B, C (Identify Type)

### Topics Covered
- `dynamic_cast` and runtime type identification (RTTI)
- Why RTTI requires at least one virtual function
- The pointer-cast vs. reference-cast idiom (`NULL` check vs. `try`/`catch`)

### Concept

`A`, `B`, and `C` are empty siblings that all inherit publicly from `Base`. The only thing giving `Base` RTTI at all is its virtual destructor:

```cpp
class Base {
public:
    virtual ~Base();
};

class A : public Base {};
```

`generate()` picks one of the three at random and returns it through a `Base*`, so the caller has no static knowledge of which concrete type it actually got:

```cpp
Base* generate(void) {
    int random = std::rand() % 3;
    switch (random) {
        case 0: return new A();
        case 1: return new B();
        case 2: return new C();
    }
}
```

`identify(Base*)` uses `dynamic_cast` to a pointer, which returns `NULL` on a failed cast — no exception needed:

```cpp
void identify(Base* p) {
    if (dynamic_cast<A*>(p))
        std::cout << "A" << std::endl;
    else if (dynamic_cast<B*>(p))
        std::cout << "B" << std::endl;
    else if (dynamic_cast<C*>(p))
        std::cout << "C" << std::endl;
    else
        std::cout << "Unknown type" << std::endl;
}
```

`identify(Base&)` can't return `NULL` — there's no such thing as a null reference — so a failed `dynamic_cast` to a reference throws `std::bad_cast` instead, and each attempt is wrapped in its own `try`/`catch`:

```cpp
void identify(Base& p) {
    try {
        (void)dynamic_cast<A&>(p);
        std::cout << "A" << std::endl;
        return;
    } catch (std::exception&) {}
    // ... same pattern for B, C
}
```

### Compilation & Usage
```bash
cd ex02 && make
./indentify
```

---

## Why static_cast / reinterpret_cast / dynamic_cast, Not C-Style Casts

A C-style cast `(T)expr` silently picks whichever of `static_cast`, `const_cast`, `reinterpret_cast`, or a combination applies, with no record of which one it chose. That's convenient and exactly the problem: the compiler can't warn about a cast that quietly reinterprets bits when you meant to convert a value, because it can't tell which one you meant either.

### The Four Named Casts

| Cast | Checked how | Used in this module for |
|---|---|---|
| `static_cast` | Compile-time, related types only | Numeric conversions between `char`/`int`/`float`/`double` (ex00) |
| `reinterpret_cast` | Not checked at all | Pointer ↔ integer reinterpretation (ex01) |
| `dynamic_cast` | Runtime, via RTTI | Downcasting through a polymorphic base (ex02) |
| `const_cast` | Compile-time, `const`/`volatile` only | Not used in this module |

Each cast makes exactly one kind of intent visible at the call site and rejects (or fails at runtime) anything else, which is precisely the safety a C-style cast throws away.

---

## Learning Path

1. **ex00**: parse text by hand and pick, for each of `char`/`int`/`float`/`double`, whether `static_cast` even produces a meaningful result.
2. **ex01**: use `reinterpret_cast` for the one job it's actually for — treating a pointer's bit pattern as an integer and back — and see why that's different from casting a value.
3. **ex02**: use `dynamic_cast` to ask "what is this, really?" at runtime, once as a pointer (get `NULL`) and once as a reference (catch `std::bad_cast`).

## Challenges

- **Float vs. double literal detection**: both require a decimal point; only the trailing `f` tells them apart, and it has to be stripped before the numeric part is validated.
- **`char` has three outcomes, not two**: printable, `Non displayable` (valid byte, unprintable), and `impossible` (out of range or NaN/inf) all need separate handling in every `convertFrom*` function.
- **`reinterpret_cast` looks trivial and isn't**: it's one line each way, but understanding *why* it's the only cast that applies here (and why it would be unsafe to serialize across process boundaries or after the original object is destroyed) is the actual point of the exercise.
- **Pointer vs. reference `dynamic_cast`**: forgetting that a failed reference cast throws instead of returning `NULL` is the most common bug in ex02.

## Tips for Success

1. In ex00, validate the literal's *textual* shape before ever calling `strtol`/`atof` — a malformed string handed to those functions doesn't fail loudly.
2. Keep the `char`/`int`/`float`/`double` overflow checks explicit and separate; folding them together tends to hide the case where a `float` is finite but still out of `int` range.
3. In ex01, resist the urge to add validation to `serialize`/`deserialize` — the exercise is about showing the cast does *no* checking, not about wrapping it in some.
4. In ex02, give `Base` a virtual destructor before anything else; without it, `dynamic_cast` won't compile against a polymorphic type at all.
5. Run under `valgrind --leak-check=full` on ex02 — `generate()` returns owned pointers that `main` is responsible for `delete`-ing.

## Resources

- [cppreference.com: static_cast](https://en.cppreference.com/w/cpp/language/static_cast)
- [cppreference.com: reinterpret_cast](https://en.cppreference.com/w/cpp/language/reinterpret_cast)
- [cppreference.com: dynamic_cast](https://en.cppreference.com/w/cpp/language/dynamic_cast)
- [cppreference.com: uintptr_t](https://en.cppreference.com/w/cpp/types/integer)
- "Effective C++" by Scott Meyers (items on casting)

## Checklist for Completion

- [ ] `ScalarConverter` correctly identifies `char`, `int`, `float`, `double`, and pseudo-literal (`nan`/`inf`) inputs
- [ ] All four output lines (`char`/`int`/`float`/`double`) are printed for every valid input, with `impossible`/`Non displayable` where appropriate
- [ ] Invalid literals print an error instead of crashing or misparsing
- [ ] `Serializer::serialize`/`deserialize` round-trip a pointer through `uintptr_t` without loss
- [ ] `identify(Base*)` returns `"Unknown type"` on a failed cast instead of crashing
- [ ] `identify(Base&)` catches `std::bad_cast` per type instead of letting it propagate
- [ ] `Base` has a virtual destructor so RTTI works at all
- [ ] No memory leaks (`generate()`-created objects are `delete`d)
- [ ] Everything compiles cleanly with `-Wall -Wextra -Werror -std=c++98`

## Author

**caida-si**
*42*
