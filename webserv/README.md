*42 Common Core project by `rsommer` and `caida-si`.*

# webserv

An HTTP/1.1 server written in C++98 using non-blocking sockets and Linux `epoll`. The server handles multiple clients in one event loop, including the pipes used for CGI programs. There is no thread for every connection.

## What it supports

- HTTP methods **GET**, **POST**, and **DELETE**, with status codes and response headers.
- Persistent HTTP/1.1 connections and chunked transfer encoding.
- Static files with MIME types, index files, and optional directory listings (`autoindex`).
- Multiple virtual servers selected by listening address/port and `server_name`.
- Per-location rules: allowed methods, document root, upload directory, redirects, body-size limits, and custom error pages.
- Multipart and raw-body uploads.
- CGI execution with request/environment data and non-blocking pipe handling.
- Basic protection against traversal through `..` in request paths, plus timeouts for inactive clients.
- Bonus: server-managed `session_id` cookies with an idle timeout, and CGI support through an additional interpreter.

The original README documents Python, PHP, and Bash CGI mappings in its example configuration. Which interpreter runs is determined by the file extension and the configured mapping, not hardcoded into the request parser.

## How requests are handled

```text
Listening sockets
     |
     v
epoll event loop -------> client state / input buffer
     |                           |
     |                      HTTP request parser
     |                           |
     |                      config / location match
     |                       /               \
     |                 static file           CGI child
     |                                          |
     +<------------- output buffers <------ CGI pipes
     |
     v
Non-blocking writes back to the client
```

Partial reads and writes matter here. A socket may deliver only part of a request, and a write may send only part of a response, so the server keeps per-client state instead of assuming one `recv` or `send` finishes a message. CGI programs have their own lifecycle and pipes, which also need to be included in the `epoll` loop.

## Build and run

Requires Linux (`epoll`), Make, and a C++98-capable compiler.

```sh
cd webserv
make
./webserv webserv.conf
```

If no config argument is supplied, the server reads `webserv.conf` from the current working directory. Makefile targets include `clean`, `fclean`, and `re`.

## Configuration

The syntax resembles Nginx, but it is parsed by this project:

```nginx
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 10M;

    error_page 404 /errors/404.html;

    location / {
        root ./www;
        index index.html;
        methods GET POST DELETE;
        autoindex off;
    }

    location /upload/ {
        root ./www;
        methods GET POST DELETE;
        upload_path ./www/upload;
        autoindex on;
    }

    location /cgi-bin/ {
        root .;
        methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php-cgi;
        cgi .sh /bin/bash;
    }

    location /old/ {
        return 301 /new/;
    }
}
```

Directives include `listen`, `server_name`, `client_max_body_size`, `location`, `root`, `index`, `methods`, `autoindex`, `upload_path`, `cgi`, `error_page`, and `return`. See the actual `webserv.conf` for the full example and the second virtual server on port 8081.

## Testing

The default configuration includes a test page, an upload location, CGI scripts, and a session demo:

```sh
curl http://localhost:8080/
curl 'http://localhost:8080/cgi-bin/test.py?name=world&foo=bar'
curl -F 'file=@/path/to/file.txt' http://localhost:8080/upload/
curl -X DELETE http://localhost:8080/upload/file.txt
curl http://localhost:8080/session
curl http://localhost:8081/
```

Also test malformed HTTP, very large bodies, slow clients, disconnected clients, and several simultaneous connections. A response being correct in the browser isn't enough if a partial write or a CGI timeout can leave a connection stuck.

## Source layout

The implementation separates configuration parsing, HTTP request parsing, response generation, clients, CGI management, and the main `epoll` server loop under `src/`. Static content is under `www/` and test CGI scripts under `cgi-bin/`.

AI tools were used as a debugging aid during development.
