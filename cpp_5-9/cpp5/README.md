# CPP Module 05: Repetition and Exceptions

CPP Module 05 introduces exception handling, C++'s mechanism for reporting and reacting to error conditions without threading error codes through every return value. The module is framed as a bureaucracy simulator: bureaucrats, the forms they sign and execute, and the interns who churn them out.

## Overview

| Exercise | Class(es) | Key Concepts |
|---|---|---|
| ex00 | `Bureaucrat` | Custom exceptions, constructor validation |
| ex01 | `Bureaucrat`, `Form` | Cross-class validation, catching in a caller |
| ex02 | `Bureaucrat`, `AForm` + 3 concrete forms | Abstract classes, Template Method |
| ex03 | `Intern` | Factory pattern without if/else-if |

**Important Constraint: C++98 Only**

Like every module up to Module 07, this one is restricted to C++98:
- No `<algorithm>`, no STL containers (`vector`, `map`, ...)
- No `auto`, no range-based `for`, no lambdas
- `const char *what() const throw()` instead of the C++11 `noexcept` spelling

Compile with:
```makefile
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
```

---

## Exercise 00: Bureaucrat

### Topics Covered
- Throwing exceptions from a constructor to reject an invalid object outright
- Nested exception classes (`Bureaucrat::GradeTooHighException`)
- Overloading `operator<<`

### Concept

A `Bureaucrat` has a name and a grade from **1 (highest) to 150 (lowest)**. The numbering is inverted from what you'd expect, and it trips up every increment/decrement call in the project:

```cpp
Bureaucrat::Bureaucrat(const std::string name, int grade) : _name(name), _grade(grade)
{
    if (grade < 1)
        throw GradeTooHighException();
    if (grade > 150)
        throw GradeTooLowException();
}

void Bureaucrat::incrementGrade()
{
    if (_grade - 1 < 1)
        throw GradeTooHighException();
    _grade--;   // incrementing the *rank* means the number goes down
}
```

If the constructor throws, the object never comes into existence, so there's nothing to clean up.

### Compilation & Usage
```bash
cd ex00 && make && ./bureaucrat
```

---

## Exercise 01: Form

### Topics Covered
- A second class with its own grade-based rules
- One object (`Bureaucrat`) driving an operation on another (`Form`) and reporting the outcome
- Catching exceptions where they're useful, not where they're thrown

### Concept

`Form` adds `_isSigned`, `_signGrade`, and `_execGrade`. Signing only succeeds if the bureaucrat's grade is numerically **≤** the required grade:

```cpp
void Form::beSigned(const Bureaucrat &bureaucrat)
{
    if (bureaucrat.getGrade() > _signGrade)
        throw GradeTooLowException();
    _isSigned = true;
}
```

`Bureaucrat::signForm()` is where the exception actually gets handled. The low-level function throws; the high-level caller decides what to print:

```cpp
void Bureaucrat::signForm(Form &form)
{
    try
    {
        form.beSigned(*this);
        std::cout << _name << " signed " << form.getName() << std::endl;
    }
    catch (std::exception &e)
    {
        std::cout << _name << " couldn't sign " << form.getName()
                  << " because " << e.what() << std::endl;
    }
}
```

### Compilation & Usage
```bash
cd ex01 && make && ./bureaucrat
```

---

## Exercise 02: AForm and Concrete Forms

### Topics Covered
- Abstract classes (pure virtual functions)
- The **Template Method** pattern
- Three concrete subclasses with different behavior, same validation

### Concept

`Form` is renamed `AForm` and becomes abstract via a pure virtual `executeForm()`. The grade checks live once, in the base class's `execute()`; each subclass only supplies the part that differs:

```cpp
void AForm::execute(const Bureaucrat &executor) const
{
    if (!_isSigned)
        throw FormNotSignedException();
    if (executor.getGrade() > _execGrade)
        throw GradeTooLowException();
    executeForm(executor);   // deferred to the concrete class
}
```

| Form | Sign grade | Exec grade | Action |
|---|---|---|---|
| `ShrubberyCreationForm` | 145 | 137 | Writes ASCII trees to `<target>_shrubbery` |
| `RobotomyRequestForm` | 72 | 45 | Drilling noise, then 50/50 success on `<target>` |
| `PresidentialPardonForm` | 25 | 5 | `<target> has been pardoned by President Zaphod Beeblebrox.` |

Each concrete class takes a single `target` string in its constructor and does nothing else beyond overriding `executeForm()`.

### Compilation & Usage
```bash
cd ex02 && make && ./bureaucrat
```

---

## Exercise 03: Intern (Factory Pattern)

### Topics Covered
- Encapsulating object creation behind a single entry point
- Avoiding an if/else-if ladder with a lookup table
- Pointers to member functions

### Concept

The subject explicitly forbids "unreadable... if/else-if" chains for `makeForm()`. Instead of branching on the string, `Intern` walks a small static table pairing each form name with a pointer-to-member-function:

```cpp
struct FormFactory
{
    const char  *name;
    AForm       *(Intern::*create)(const std::string &target);
};

FormFactory factories[] = {
    {"shrubbery creation",  &Intern::createShrubberyForm},
    {"robotomy request",    &Intern::createRobotomyForm},
    {"presidential pardon", &Intern::createPresidentialForm},
    {NULL, NULL}
};
```

Adding a new form type later means adding one row to the table. `makeForm()` itself never grows:

```cpp
Intern someRandomIntern;
AForm *rrf = someRandomIntern.makeForm("robotomy request", "Bender");
```

An unrecognized name throws `Intern::UnknownFormException` rather than returning `NULL`, so a caller can't accidentally dereference a bad pointer.

### Compilation & Usage
```bash
cd ex03 && make && ./bureaucrat
```

---

## Why Exceptions Matter

Error codes require the caller to check a return value at every level of the call stack, and it's always possible to forget. Exceptions propagate automatically until something catches them, which makes silently ignoring an error much harder. They also let constructors report failure at all, since a constructor has no return value to repurpose as a status code.

### Exceptions vs Error Codes

| Aspect | Exceptions | Error Codes |
|---|---|---|
| Separation from normal logic | Yes (handling lives in `catch`) | No (mixed into the happy path) |
| Propagation | Automatic, up the call stack | Manual, checked at every level |
| Can be silently ignored | No (uncaught means terminate) | Yes |
| Constructors can report failure | Yes | No (no return value) |
| Cost | Only on the throw path | Constant checking overhead |

In this project specifically: an invalid grade is a truly exceptional condition (not something that happens on every call), so paying for a `throw` only when it actually occurs is the right trade-off.

---

## Learning Path

1. **ex00**: get comfortable with a constructor that can fail and a `try`/`catch` around it.
2. **ex01**: practice catching an exception in the *caller* rather than where it's thrown. That's where you have enough context to react usefully.
3. **ex02**: pull the validation logic that's now duplicated between forms up into an abstract base, and let each subclass fill in only its own action.
4. **ex03**: swap the temptation of an if/else-if ladder for a table-driven dispatch.

## Challenges

- **Grade direction**: 1 is the top, 150 is the bottom, so every `>`/`<` comparison in the project has to be read backwards from intuition.
- **Where to validate**: `AForm::execute()` checks signing and grade once; getting this logic duplicated into each concrete form instead is the most common way to make ex02 messier than it needs to be.
- **Memory ownership**: `Intern::makeForm()` returns a raw `AForm *` (no smart pointers in C++98). Every call site is responsible for `delete`-ing it.

## Tips for Success

1. Validate in the constructor and throw immediately. Don't let an invalid `Bureaucrat` or `Form` exist even momentarily.
2. Keep exception classes nested inside the class they belong to (`Bureaucrat::GradeTooHighException` vs `AForm::GradeTooHighException`); the names match, but the types and scope are different.
3. Never let a destructor throw; it's what makes stack unwinding during exception propagation safe.
4. Test the failure paths, not just the happy path: wrong grade at construction, signing without enough rank, executing unsigned, executing with insufficient rank, an unknown form name.
5. Run under `valgrind --leak-check=full`. It's easy to leak a form returned by `Intern` if a `delete` is forgotten after an early `throw`.

## Resources

- [cppreference.com: Exceptions](https://en.cppreference.com/w/cpp/language/exceptions)
- [cppreference.com: std::exception](https://en.cppreference.com/w/cpp/error/exception)
- [Refactoring.Guru: Template Method](https://refactoring.guru/design-patterns/template-method)
- [Refactoring.Guru: Factory Method](https://refactoring.guru/design-patterns/factory-method)
- "Effective C++" by Scott Meyers (items on exceptions and resource management)

## Checklist for Completion

- [ ] `Bureaucrat` throws on out-of-range grade at construction, increment, and decrement
- [ ] `Form`/`AForm` throws on out-of-range sign/exec grade at construction
- [ ] `beSigned()` and `execute()` throw the correct, specific exception for each failure
- [ ] `Bureaucrat::signForm()` / `executeForm()` print success or the exact failure reason
- [ ] All three concrete forms implemented with the grades specified by the subject
- [ ] `Intern::makeForm()` uses a table/dispatch, not an if/else-if chain
- [ ] No memory leaks (`Intern`-created forms are `delete`d)
- [ ] Everything compiles cleanly with `-Wall -Wextra -Werror -std=c++98`

## Author

**caida-si**
*42*
