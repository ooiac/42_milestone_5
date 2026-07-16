# 42 School: Milestone 5

Welcome to **Milestone 5** of the 42 School curriculum! This milestone finishes the C++98 track (CPP Modules 5 through 9) and then pushes into two much larger builds: a production-style **HTTP/1.1 server** written from scratch in C++98, and a fully containerized **Docker infrastructure** running a real WordPress stack. Exam training for Rank 05/06 rounds out the repository.

## Overview

| Project | Type | Language | Key Concepts |
|---------|------|----------|--------------|
| **CPP Modules (5-9)** | Learning Modules | C++98 | Exceptions, casts (`static`/`reinterpret`/`dynamic`), templates, STL containers/iterators/algorithms, real-world STL programs |
| **webserv** | HTTP Server | C++98 | `epoll`-based non-blocking I/O, HTTP/1.1, CGI/1.1, virtual hosts, sessions |
| **Inception** | DevOps / Infrastructure | Docker | Docker Compose, multi-container orchestration, reverse proxy, TLS, secrets |
| **Exam Rank 5 Training** | Exam Prep | C++ / C | Operator overloading (Rank 05), board simulation with a strict function whitelist (Rank 06) |

---

## Component 1: C++ Modules (CPP 5-9)

### What They Are

The second half of the CPP modules, picking up where Milestone 4's CPP 0-4 left off. Where 0-4 built the OOP fundamentals (classes, inheritance, polymorphism), 5-9 spend that foundation on **exception-safe design**, **the four named casts**, **templates**, and finally the **STL**: first its containers and algorithms in isolation, then applied to three realistic programs. Each module has its own detailed README with full code walkthroughs; this section is the map.

**Important Constraint: C++98 Only**

All five modules are restricted to the **C++98 standard**, same as 0-4:
- No `auto`, no range-based `for`, no lambdas
- No C++11 STL additions (`unordered_map`, `array`, move semantics)
- Manual parsing (`strtol`/`strtof`) instead of `std::stoi`/`std::stof`

```makefile
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
```

### [CPP05: Repetition and Exceptions](cpp_5-9/cpp5/README.md)

A bureaucracy simulator that teaches exception handling end to end: throwing from a constructor to reject invalid state outright, catching where the context to react actually exists, and an abstract `AForm` base with three concrete forms sharing one Template Method. Finishes with `Intern`, a factory that dispatches on a lookup table instead of an if/else-if chain.

### [CPP06: Type Conversions](cpp_5-9/cpp6/README.md)

The four named casts, each used for exactly the job it's suited to: `static_cast` for numeric conversions with hand-rolled literal parsing (`ScalarConverter`), `reinterpret_cast` for a pointer ↔ `uintptr_t` round trip (`Serializer`), and `dynamic_cast` for RTTI-based downcasting, once as a pointer (`NULL` on failure) and once as a reference (`std::bad_cast` on failure).

### [CPP07: Templates](cpp_5-9/cpp7/README.md)

Function templates (`swap`/`min`/`max`), a template taking a function pointer (`iter`, overloaded on callback constness), and a full class template (`Array<T>`) with deep-copy semantics and a bounds-checked `operator[]`.

### [CPP08: Templated Containers, Iterators, Algorithms](cpp_5-9/cpp8/README.md)

The STL unlocked: `easyfind` generic over any container exposing `begin()`/`end()`, `Span` wrapping a capacity-limited `std::vector` with `std::sort`/`std::min_element`/`std::max_element`, and `MutantStack<T>`, a `std::stack` subclass that reaches into the adaptor's protected underlying container to add iteration it doesn't natively support.

### [CPP09: STL in Practice](cpp_5-9/cpp9/README.md)

Three self-contained programs, each picking the STL container that matches its problem: `BitcoinExchange` (`std::map` + `upper_bound` for nearest-date lookup), `RPN` (`std::stack` for postfix evaluation), and `PmergeMe` (the Ford-Johnson merge-insertion sort with a Jacobsthal-ordered insertion step, implemented against both `std::vector` and `std::deque` and timed head-to-head).

### C++98 vs Modern C++

| Feature | C++98 | Modern C++ (11+) |
|---------|-------|-------------------|
| Casting | 4 named casts, explicit intent | Same casts, plus `std::any`/`std::variant` for some use cases |
| Generic code | Templates, no concepts | Templates + `concepts` (C++20) |
| Container iteration | Hand-written iterator loops | Range-based `for` |
| Literal parsing | `strtol`/`strtof` + manual validation | `std::stoi`/`std::stof` (throw on failure) |

### Compilation & Usage

```bash
cd cpp_5-9/cpp7/ex02 && make && ./array
```
Every exercise follows the same pattern: `cd` into `cppN/exXX`, `make`, then run the produced binary.

---

## Component 2: webserv

### What It Is

**webserv** is a non-blocking HTTP/1.1 server built around a single `epoll` instance that multiplexes every client socket and every CGI pipe. It's the 42 curriculum's "build your own nginx" project: no thread pool, no blocking reads, one event loop.

### Key Features

- **Non-blocking I/O** via one `epoll` fd for all sockets and CGI pipes
- **HTTP/1.1**: keep-alive, chunked transfer encoding, correct status codes
- **Virtual hosts**: multiple `server` blocks routed by `server_name`
- **CGI/1.1**: Python and PHP scripts with full environment variable support
- **File uploads**: multipart/form-data and raw-body uploads
- **Directory listing (autoindex)**, custom error pages, `return`-style redirects
- **Path traversal protection** and 30-second client timeouts
- **Bonus**: server-generated session cookies (in-memory, 10-minute expiry) and a second CGI interpreter (Bash) wired up alongside Python

### Example Configuration

```nginx
server {
    listen 8080;
    server_name localhost;

    location /cgi-bin/ {
        methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .sh /bin/bash;   # bonus: second interpreter
    }
}
```

### Compilation & Usage

```bash
cd webserv
make
./webserv webserv.conf
curl http://localhost:8080/
```

Full architecture, project structure, and testing instructions live in [webserv/README.md](webserv/README.md).

---

## Component 3: Inception

### What It Is

**Inception** is a from-scratch Docker infrastructure: a full WordPress stack (nginx, WordPress/PHP-FPM, MariaDB) plus four bonus services, each in its own container built from `debian:bookworm`. No pre-built images are pulled from Docker Hub.

### Architecture

```
  HTTPS :443 ──► nginx ──► wordpress (php-fpm :9000)
                    │   └──► adminer (:8080)
                    │   └──► static-site (:80)
                    │
  FTP :21 ─────────► ftp
                    │   wordpress ──► mariadb (:3306)
                    │   wordpress ──► redis (:6379)
  HTTP :8080 ──────► filebrowser
```

### Key Features

- **Nginx**: reverse proxy, TLS 1.2/1.3 (self-signed), the only exposed HTTP entry point
- **WordPress + MariaDB**: the mandatory stack, wired over a user-defined Docker bridge network
- **Bonus services**: Redis (object cache), vsftpd (FTP into the WordPress volume), a static portfolio site, Adminer, and File Browser
- **Persistence**: named volumes bound to the host filesystem, not container-local storage
- **Secrets**: passwords and credentials passed via Docker secrets, not plain environment variables

### Compilation & Usage

```bash
cd Inception
make
```
Full setup steps (hosts file, `.env`, secrets) are in [Inception/README.md](Inception/README.md), with deeper developer/user docs in `DEV_DOC.md` and `USER_DOC.md`.

---

## Component 4: Exam Rank 5 Training

### What It Is

Timed-exam practice for the two ranks this milestone's projects are certified under. Unlike the other components, these aren't submitted 42 projects. They're rehearsal for the supervised, no-internet exam environment: a small spec, a strict function whitelist, and a fixed time limit.

### Level 1: vect2 (Rank 05)

A 2D integer vector class supporting `+`, `-`, scalar `*`, an unchecked `operator[]`, and a stream-insertion `operator<<` that must format identically to manually printing `{v[0], v[1]}`. The exercise is really about getting operator overloading syntax right under time pressure, not algorithmic complexity.

### Level 2: life (Rank 06)

A C program (`./life width height iterations`) that reads pen-drawing commands (`w`/`a`/`s`/`d` to move, `x` to toggle drawing) from stdin to build an initial board, then runs Conway's Game of Life for the given number of iterations and prints the result. Allowed functions are limited to `atoi`, `read`, `putchar`, `malloc`, `calloc`, `realloc`, `free`. No `printf`, no libft.

### Compilation & Usage

```bash
cd exam_rank_5_training/level_1/vect2 && c++ -Wall -Wextra -Werror -std=c++98 *.cpp -o vect2 && ./vect2
cd exam_rank_5_training/level_2/life && gcc -Wall -Wextra -Werror life.c -o life && ./life 20 10 5
```

---

## Curriculum Context

| Milestone | Projects | Focus |
|-----------|----------|-------|
| 0 | libft | C foundation |
| 1 | ft_printf, get_next_line | C I/O |
| 2 | push_swap, minitalk, fract-ol | C algorithms & graphics |
| 3 | minishell, philosophers | C systems programming |
| 4 | cpp 0-4, cub3D, netpractice | C++ OOP, advanced graphics, networking |
| **5** | **cpp 5-9, webserv, Inception** | **Advanced C++ (templates/STL), network servers, DevOps/infrastructure** |

Milestone 5 is where the curriculum stops teaching C++ in isolated exercises and starts asking for it inside a large, long-lived system (`webserv`). Inception shifts entirely out of language-level work into infrastructure and orchestration.

---

## Tips for Success

### CPP Modules
1. Read each module's own README before starting its exercises. The "Challenges" section for each one calls out the specific bug every past attempt hits first.
2. Run `valgrind --leak-check=full` after every exercise, not just at the end of a module; leaks compound and get harder to isolate later.
3. Keep exception classes nested inside the class they belong to, and never let a destructor throw.

### webserv
1. Build the HTTP request parser as an explicit state machine from the start. Retrofitting one onto ad-hoc string parsing is far more painful than starting there.
2. Test with real clients early (`curl`, a browser, `ab`/`siege` for load), not only a hand-rolled test script. Malformed real-world requests surface parser bugs synthetic tests miss.
3. Keep everything behind the single `epoll` loop; the moment a blocking call sneaks in, every other connection stalls with it.

### Inception
1. Get one container (nginx) fully working and reachable before adding the next. Debugging four containers that don't talk to each other at once is much harder than one at a time.
2. Never bake passwords into a Dockerfile or commit them; use Docker secrets or an untracked `.env`.
3. `docker compose logs -f <service>` is the fastest way to find out why a container keeps restarting.

### Exam Training
1. Practice under the exact constraint set (allowed functions, no internet, fixed time). The exam penalizes reaching for a convenience function that isn't on the list.
2. Read the whole subject before writing any code; Rank 06 exercises especially tend to bury an edge case (like the pen never toggling "down") in the middle of the spec.

---

## Resources

### CPP Modules
- [cppreference.com](https://en.cppreference.com/), the canonical reference, including for C++98-era usage
- "Effective C++" and "Effective STL" by Scott Meyers

### webserv
- [RFC 7230](https://tools.ietf.org/html/rfc7230) and [RFC 7231](https://tools.ietf.org/html/rfc7231) (HTTP/1.1)
- [RFC 3875](https://tools.ietf.org/html/rfc3875) (CGI/1.1)
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)

### Inception
- [Docker documentation](https://docs.docker.com/) and [Docker Compose documentation](https://docs.docker.com/compose/)
- [MariaDB knowledge base](https://mariadb.com/kb/en/)
- [Nginx documentation](https://nginx.org/en/docs/)

---

## Checklist for Completion

### CPP Modules
- [ ] All exercises in cpp5-cpp9 compile cleanly with `-Wall -Wextra -Werror -std=c++98`
- [ ] No memory leaks in any exercise (`valgrind --leak-check=full` clean)
- [ ] Each module's own checklist (see its README) is satisfied

### webserv
- [ ] Handles multiple virtual servers and concurrent connections without blocking
- [ ] CGI execution works for both configured interpreters
- [ ] File upload, directory listing, and custom error pages all function
- [ ] Server survives malformed requests and slow/partial clients without crashing or hanging

### Inception
- [ ] All mandatory services (nginx, WordPress, MariaDB) start via `docker compose up` and are reachable
- [ ] TLS termination works and HTTP is not exposed
- [ ] Data persists across `docker compose down && docker compose up`
- [ ] All bonus services configured start correctly alongside the mandatory stack

### Exam Rank 5 Training
- [ ] `vect2` supports `+`, `-`, scalar `*`, `operator[]`, and `operator<<` matching the spec's exact output format
- [ ] `life` respects the allowed-functions whitelist and correctly simulates the given number of Game of Life iterations

---

**Milestone 5 is where the projects stop being exercises and start being systems: a server that has to stay up, an infrastructure that has to stay reproducible. Good luck!**
