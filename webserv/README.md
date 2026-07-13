*This project has been created as part of the 42 curriculum by rsommer, caida-si.*

# webserv

A fully-featured HTTP/1.1 server written in C++98, built for the 42 school webserv project.

## Description

webserv is a non-blocking HTTP server that uses a single epoll instance to handle all I/O operations. It supports multiple virtual servers, static file serving, CGI execution, file uploads, and directory listing. The server is designed to handle concurrent connections efficiently without threads or blocking I/O.

**Key features:**

- **Non-blocking I/O**: Single epoll file descriptor manages all sockets and CGI pipes
- **HTTP/1.1**: Persistent connections (keep-alive), chunked transfer encoding, proper status codes
- **Virtual hosts**: Multiple `server` blocks with `server_name` based routing
- **Static files**: Serves HTML, CSS, JS, images, and any file type with correct MIME types
- **CGI/1.1**: Executes Python and PHP scripts with full environment variable support
- **File uploads**: Multipart/form-data parsing, raw body uploads, saved to configurable directory
- **Directory listing**: Autoindex with styled HTML output when no index file is found
- **Custom error pages**: Per-status-code error page configuration in the config file
- **Redirects**: `return` directive with configurable status codes (301, 302, etc.)
- **Path traversal protection**: Blocks `..` in URI paths
- **Client timeouts**: Inactive connections closed after 30 seconds
- **Sessions & cookies** *(bonus)*: server-generated `session_id` cookie, tracked in-memory, expires after 10 minutes of inactivity
- **CGI/1.1 in two languages** *(bonus)*: Python and Bash interpreters both wired up and working

## Instructions

### Building

```bash
make
```

Requires a C++98-compatible compiler (tested with `c++ -Wall -Wextra -Werror -std=c++98`).

### Running

```bash
./webserv [config_file]
```

If no config file is specified, `webserv.conf` in the current directory is used.

```bash
./webserv webserv.conf
```

### Configuration File

The configuration file uses nginx-inspired syntax:

```nginx
server {
    listen 8080;                          # port (or host:port)
    server_name localhost;                # virtual host names
    client_max_body_size 10M;             # max body size (K/M/G suffixes)

    error_page 404 /errors/404.html;      # custom error pages

    location / {
        root ./www;                       # document root
        index index.html;                 # default index file
        methods GET POST DELETE;          # allowed methods
        autoindex off;                    # directory listing
    }

    location /upload/ {
        methods POST GET DELETE;
        root ./www;
        upload_path ./www/upload;         # save uploaded files here
        autoindex on;
    }

    location /cgi-bin/ {
        methods GET POST;
        root .;
        cgi .py /usr/bin/python3;         # extension -> interpreter
        cgi .php /usr/bin/php-cgi;
        cgi .sh /bin/bash;                # a second, independent interpreter
    }

    location /old/ {
        return 301 /new/;                 # redirect
    }
}
```

### Makefile Targets

| Target  | Description                                 |
|---------|---------------------------------------------|
| `all`   | Build the `webserv` executable              |
| `clean` | Remove object files                         |
| `fclean`| Remove object files and the executable      |
| `re`    | Full rebuild (fclean + all)                 |

### Testing

Open a browser and navigate to:
- `http://localhost:8080/` — Main page
- `http://localhost:8080/upload/` — File upload and directory listing
- `http://localhost:8080/cgi-bin/test.py` — CGI environment viewer (Python)
- `http://localhost:8080/cgi-bin/sysinfo.sh` — CGI environment viewer (Bash, bonus)
- `http://localhost:8080/cgi-bin/upload.py` — CGI file upload handler
- `http://localhost:8080/session` — session & cookies demo (bonus): reload to watch the visit counter go up
- `http://localhost:8081/` — Second virtual server (port 8081)

Using curl:

```bash
# GET request
curl http://localhost:8080/

# POST file upload
curl -F "file=@/path/to/file.txt" http://localhost:8080/upload/

# DELETE request
curl -X DELETE http://localhost:8080/upload/file.txt

# CGI with query string
curl "http://localhost:8080/cgi-bin/test.py?name=world&foo=bar"
```

### Project Structure

```
webserv/
├── src/
│   ├── main.cpp          # Entry point, signal handling
│   ├── Config.hpp/.cpp   # Configuration file parser
│   ├── HttpRequest.hpp/.cpp   # HTTP request parser (state machine)
│   ├── HttpResponse.hpp/.cpp  # HTTP response builder
│   ├── CgiHandler.hpp/.cpp    # CGI fork/exec and pipe management
│   ├── Client.hpp/.cpp        # Per-connection state
│   ├── Server.hpp/.cpp        # epoll event loop and request routing
│   ├── SessionManager.hpp/.cpp # In-memory sessions & cookies (bonus)
│   └── Utils.hpp/.cpp         # Utility functions
├── www/
│   ├── index.html        # Main static page
│   ├── errors/           # Custom error pages
│   └── upload/           # Upload destination directory
├── cgi-bin/
│   ├── test.py            # CGI environment test script (Python)
│   ├── sysinfo.sh         # CGI environment test script (Bash, bonus)
│   └── upload.py          # CGI file upload handler
├── Makefile
└── webserv.conf          # Default configuration
```

## Bonus

### Sessions & cookies

`SessionManager` keeps an in-memory `map<session_id, SessionData>` inside the
`Server`. On every request, `Server::processRequest()` reads the `Cookie:`
header and either finds the matching session or creates a new one; if a new
one was created, a `Set-Cookie: session_id=...; Path=/; HttpOnly; Max-Age=600`
header is attached to that response (for both regular and CGI-generated
responses). Sessions idle for more than 10 minutes are dropped the next time
the timeout sweep runs.

`GET /session` is a small built-in route (no config or CGI needed) that shows
the session id and a per-session visit counter — reload the page to watch it
increase, or hit it from a different client/cookie jar to see an independent
session start at 1.

### CGI in a second language

The CGI mechanism forks, execs, and pipes stdin/stdout generically — it was
never tied to Python specifically. `webserv.conf` registers a second
interpreter, `cgi .sh /bin/bash;`, and `cgi-bin/sysinfo.sh` is a real Bash CGI
script (handles GET and POST, reads `CONTENT_LENGTH`/stdin, prints CGI env
vars) proving the second language actually works end to end, not just in
config.

## Resources

- [RFC 7230 — HTTP/1.1 Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 — HTTP/1.1 Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [RFC 3875 — The Common Gateway Interface (CGI/1.1)](https://tools.ietf.org/html/rfc3875)
- [epoll(7) — Linux man page](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [nginx configuration documentation](https://nginx.org/en/docs/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- **AI assistance**: This project was developed with the help of Claude (Anthropic) for debugging of the C++98 implementation.
