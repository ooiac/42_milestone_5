# Milestone 5 — C++05–09, webserv, Inception

This repository contains the second half of the C++98 modules, two larger systems projects, and my Rank 05/06 exam practice.

## C++ modules 05–09

The modules live in [`cpp_5-9/`](cpp_5-9/), with a separate README and Makefile for each exercise. They all use C++98 (`-Wall -Wextra -Werror -std=c++98`).

| Module | Work | Main implementation details |
| --- | --- | --- |
| [05 — Exceptions](cpp_5-9/cpp5/README.md) | `Bureaucrat`, `Form`, `AForm`, `Intern` | Valid grade ranges, exceptions, abstract forms, factory lookup. |
| [06 — Casts](cpp_5-9/cpp6/README.md) | `ScalarConverter`, `Serializer`, RTTI | Literal classification, `static_cast`, pointer round-tripping, `dynamic_cast`. |
| [07 — Templates](cpp_5-9/cpp7/README.md) | `swap`, `min`, `max`, `iter`, `Array<T>` | Template instantiation, callback types, deep-copy and bounds checks. |
| [08 — STL](cpp_5-9/cpp8/README.md) | `easyfind`, `Span`, `MutantStack<T>` | Iterators, container algorithms, exposing a stack adaptor's iterators. |
| [09 — STL applications](cpp_5-9/cpp9/README.md) | `BitcoinExchange`, `RPN`, `PmergeMe` | Date-to-rate map lookup, stack evaluation, Ford–Johnson sorting. |

For example:

```sh
cd cpp_5-9/cpp7/ex02
make
./array
```

The main shift across these modules is from hand-managed class behavior to generic code and choosing the appropriate STL container. The individual READMEs explain the algorithms and failure cases.

## webserv

Source: [`webserv/`](webserv/) · [Detailed README](webserv/README.md)

An HTTP/1.1 server in C++98 using Linux `epoll` for non-blocking I/O. A single event loop tracks client sockets and CGI pipes rather than starting one thread per connection.

The server parses requests and builds responses, supports persistent connections, chunked transfer, static files, directory listing, redirects, and custom error pages. The configuration file defines multiple virtual servers and per-location rules. GET, POST, and DELETE are supported, including file uploads. CGI programs receive request data and environment variables through the child process, and the bonus adds session cookies and another CGI interpreter.

```text
client -> listening socket -> epoll loop
                                 |
                         request parser
                                 |
                         location / routing
                          /             \
                  static response     CGI child
                                          |
                                     pipes -> epoll
```

```sh
cd webserv
make
./webserv webserv.conf
```

The detailed README contains the supported config directives, sample requests, and endpoints. Unlike a framework application, the server handles its own HTTP parsing, response generation, and I/O state.

## Inception

Source: [`Inception/`](Inception/) · [Detailed README](Inception/README.md)

A Docker Compose infrastructure using Debian-based images built by the project rather than prebuilt service images. Mandatory services are Nginx (TLS reverse proxy), WordPress/PHP-FPM, and MariaDB. Bonus services include Redis, vsftpd, a static page, Adminer, and File Browser.

```text
HTTPS :443 -> Nginx -> WordPress (PHP-FPM) -> MariaDB
                    \-> static site / Adminer
             WordPress -> Redis
FTP :21 -> vsftpd -> WordPress volume
:8080 -> File Browser
```

Containers communicate over a Docker bridge network. Credentials are provided through secrets, and data persists in volumes mounted to the host, so restarting or rebuilding a container does not wipe the database or site files.

```sh
cd Inception
# Configure the local hostname, srcs/.env, and secrets first.
make
```

The internal README and `DEV_DOC.md` / `USER_DOC.md` cover setup and maintenance. Be careful with the Makefile's cleanup targets: they can clean Docker resources outside this project.

## Exam practice

[`exam_rank_5_training/`](exam_rank_5_training/) contains timed-exam practice, not extra milestone projects.

- **Rank 05 (`vect2`):** a two-dimensional integer vector class implementing arithmetic, indexing, and stream output through overloaded operators.
- **Rank 06 (`life`):** a Conway's Game of Life program that reads drawing commands from stdin, builds the starting grid, and simulates the requested number of generations. The allowed functions are deliberately limited.

The practice code is kept here for reference alongside the milestone work.
