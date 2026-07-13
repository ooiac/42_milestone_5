#include "Server.hpp"
#include "Utils.hpp"
#include "HttpResponse.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <algorithm>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <fstream>

// ─── helpers ────────────────────────────────────────────────────────────────

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Parse a dotted-quad IPv4 string into a network-byte-order address.
// Returns INADDR_ANY on any malformed input. Uses only htonl (allowed);
// inet_pton/inet_addr are not in the allowed function list.
static in_addr_t parseIPv4(const std::string &host) {
    unsigned long parts[4] = {0, 0, 0, 0};
    int idx = 0;
    unsigned long cur = 0;
    bool hasDigit = false;

    for (size_t i = 0; i <= host.size(); ++i) {
        if (i < host.size() && host[i] >= '0' && host[i] <= '9') {
            cur = cur * 10 + static_cast<unsigned long>(host[i] - '0');
            hasDigit = true;
        } else if (i == host.size() || host[i] == '.') {
            if (!hasDigit || idx > 3 || cur > 255) return INADDR_ANY;
            parts[idx++] = cur;
            cur = 0;
            hasDigit = false;
        } else {
            return INADDR_ANY;
        }
    }
    if (idx != 4) return INADDR_ANY;
    return htonl(static_cast<uint32_t>((parts[0] << 24) | (parts[1] << 16) |
                                       (parts[2] << 8) | parts[3]));
}

// Format a network-byte-order IPv4 address as a dotted-quad string.
// Uses only ntohl (allowed); inet_ntop/inet_ntoa are not in the allowed list.
static std::string formatIPv4(const struct in_addr &a) {
    unsigned long h = ntohl(a.s_addr);
    std::ostringstream ss;
    ss << ((h >> 24) & 0xFF) << "." << ((h >> 16) & 0xFF) << "."
       << ((h >> 8) & 0xFF) << "." << (h & 0xFF);
    return ss.str();
}

// ─── ctor / dtor ────────────────────────────────────────────────────────────

Server::Server() : _epollFd(-1), _maxBodyCeiling(1024 * 1024) {}

Server::~Server() {
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
    for (std::map<int, size_t>::iterator it = _listenFds.begin(); it != _listenFds.end(); ++it)
        close(it->first);
    if (_epollFd >= 0) close(_epollFd);
}

// ─── init ───────────────────────────────────────────────────────────────────

bool Server::init(const Config &cfg) {
    _servers = cfg.getServers();

    // Largest configured body limit across all servers *and* locations
    // (a location's client_max_body_size may exceed its server's default).
    // Used as an upper bound while parsing, before the Host header and
    // request path identify exactly which server/location applies; the
    // precise limit is re-enforced in processRequest().
    _maxBodyCeiling = 1024 * 1024;
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].clientMaxBodySize > _maxBodyCeiling)
            _maxBodyCeiling = _servers[i].clientMaxBodySize;
        for (size_t j = 0; j < _servers[i].locations.size(); ++j)
            if (_servers[i].locations[j].clientMaxBodySize > _maxBodyCeiling)
                _maxBodyCeiling = _servers[i].locations[j].clientMaxBodySize;
    }

    _epollFd = epoll_create(1);
    if (_epollFd < 0) {
        std::cerr << "epoll_create: " << strerror(errno) << std::endl;
        return false;
    }

    for (size_t i = 0; i < _servers.size(); ++i) {
        const ServerConfig &srv = _servers[i];

        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::cerr << "socket: " << strerror(errno) << std::endl;
            return false;
        }

        int opt = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        setNonBlocking(fd);

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(static_cast<uint16_t>(srv.port));
        if (srv.host.empty() || srv.host == "0.0.0.0")
            addr.sin_addr.s_addr = INADDR_ANY;
        else
            addr.sin_addr.s_addr = parseIPv4(srv.host);

        if (bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "bind " << srv.host << ":" << srv.port
                      << ": " << strerror(errno) << std::endl;
            close(fd);
            return false;
        }
        if (listen(fd, SOMAXCONN) < 0) {
            std::cerr << "listen: " << strerror(errno) << std::endl;
            close(fd);
            return false;
        }

        if (!addToEpoll(fd, EPOLLIN)) return false;
        _listenFds[fd] = i;
        std::cout << "Listening on " << srv.host << ":" << srv.port << std::endl;
    }
    return true;
}

// ─── epoll helpers ──────────────────────────────────────────────────────────

bool Server::addToEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = events;
    ev.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        std::cerr << "epoll_ctl ADD fd=" << fd << ": " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool Server::modEpoll(int fd, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = events;
    ev.data.fd = fd;
    if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0) {
        std::cerr << "epoll_ctl MOD fd=" << fd << ": " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void Server::removeFromEpoll(int fd) {
    epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);
}

// ─── main event loop ────────────────────────────────────────────────────────

void Server::run() {
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(_epollFd, events, MAX_EVENTS, 5000);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait: " << strerror(errno) << std::endl;
            break;
        }

        for (int n = 0; n < nfds; ++n) {
            int fd = events[n].data.fd;

            if (_listenFds.count(fd)) {
                acceptClient(fd);
            } else if (_cgiWriteFds.count(fd)) {
                writeCgi(fd);
            } else if (_cgiReadFds.count(fd)) {
                readCgi(fd);
            } else if (_clients.count(fd)) {
                if (events[n].events & (EPOLLIN | EPOLLHUP | EPOLLERR)) {
                    if (_clients[fd]->getState() == CLIENT_READING)
                        readClient(fd);
                    else
                        writeClient(fd);
                } else if (events[n].events & EPOLLOUT) {
                    writeClient(fd);
                }
            }
        }

        checkTimeouts();

        // Reap finished CGI children
        int status;
        while (waitpid(-1, &status, WNOHANG) > 0) {}
    }
}

// ─── accept ─────────────────────────────────────────────────────────────────

void Server::acceptClient(int listenFd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    // One accept per readiness notification. The listening socket is watched
    // in level-triggered mode, so any further pending connections trigger
    // another epoll event. This avoids looping on accept (which would require
    // inspecting errno to detect EAGAIN).
    int clientFd = accept(listenFd, reinterpret_cast<struct sockaddr*>(&addr), &len);
    if (clientFd < 0)
        return;
    setNonBlocking(clientFd);

    Client *client = new Client(clientFd, formatIPv4(addr.sin_addr), listenFd);
    _clients[clientFd] = client;
    addToEpoll(clientFd, EPOLLIN);
}

// ─── read from client ───────────────────────────────────────────────────────

// Feeds data (possibly none — see below) into the client's request parser
// and processes or error-responds if a full request is now buffered.
//
// Called with real data from readClient(). Also called with an empty
// buffer right after a keep-alive reset: a pipelined second request can
// arrive concatenated with the first in the very same recv(), and
// HttpRequest keeps any such leftover bytes across reset() rather than
// discarding them — but since no *new* socket data follows, nothing would
// otherwise trigger epoll to re-check it, so we check it immediately here.
void Server::feedAndProcess(Client *client, const char *data, size_t len) {
    bool ok = client->getRequest().feed(data, len, _maxBodyCeiling);
    if (!ok || client->getRequest().isError()) {
        int errCode = client->getRequest().getErrorCode();
        if (errCode == 0) errCode = 400;
        std::string resp = makeErrorResponse(client, errCode, NULL);
        client->setSendBuffer(resp);
        client->setState(CLIENT_SENDING);
        client->setKeepAlive(false);
        modEpoll(client->getFd(), EPOLLOUT);
        return;
    }

    if (client->getRequest().getState() == REQ_COMPLETE)
        processRequest(client);
}

void Server::readClient(int fd) {
    Client *client = _clients[fd];
    client->updateActivity();

    // One recv per readiness notification. The socket is watched in
    // level-triggered mode, so any remaining buffered data triggers another
    // epoll event. We must not inspect errno after recv; a non-positive
    // return simply means the connection is closed or broken.
    char buf[READ_BUFSIZE];
    ssize_t n = recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) {
        closeClient(fd);
        return;
    }

    // Parse with the largest configured body limit as an upper bound; the
    // exact per-server limit is re-checked in processRequest() once the
    // Host header identifies the target server.
    feedAndProcess(client, buf, static_cast<size_t>(n));
}

// ─── write to client ────────────────────────────────────────────────────────

void Server::writeClient(int fd) {
    Client *client = _clients[fd];
    client->updateActivity();

    // One send per writability notification. The socket is watched in
    // level-triggered EPOLLOUT mode while data is pending, so the kernel
    // reports writability again as buffer space frees up. Because we only
    // send when epoll says the socket is writable, send never needs an
    // errno check: a non-positive return means the connection is broken.
    if (client->hasPendingSend()) {
        const std::string &buf = client->getSendBuffer();
        ssize_t n = send(fd, buf.c_str(), buf.size(), MSG_NOSIGNAL);
        if (n <= 0) {
            closeClient(fd);
            return;
        }
        client->consumeSendBuffer(static_cast<size_t>(n));
    }

    if (client->hasPendingSend())
        return; // more to send; EPOLLOUT will fire again

    if (client->isKeepAlive() && client->getState() == CLIENT_SENDING) {
        // Reset for next request. Any bytes of a pipelined next request
        // that arrived alongside this one are preserved in the request's
        // buffer across reset() — check for them right away.
        client->getRequest().reset();
        client->setState(CLIENT_READING);
        modEpoll(fd, EPOLLIN);
        feedAndProcess(client, "", 0);
    } else {
        closeClient(fd);
    }
}

// ─── CGI write (server -> CGI stdin) ────────────────────────────────────────

void Server::writeCgi(int fd) {
    Client *client = _cgiWriteFds[fd];
    if (!client) { removeFromEpoll(fd); close(fd); return; }
    client->updateActivity();

    const std::string &body = client->getCgiBodyToWrite();
    if (body.empty()) {
        // Done writing body — close write end
        removeFromEpoll(fd);
        _cgiWriteFds.erase(fd);
        client->getCgi().closeWritePipe();
        return;
    }

    // One write per writability notification (level-triggered EPOLLOUT).
    // No errno inspection: a non-positive return means the pipe is broken
    // (e.g. the CGI process exited), so we stop feeding its stdin.
    ssize_t n = write(fd, body.c_str(), body.size());
    if (n <= 0) {
        removeFromEpoll(fd);
        _cgiWriteFds.erase(fd);
        client->getCgi().closeWritePipe();
        return;
    }
    client->consumeCgiBody(static_cast<size_t>(n));
    if (client->getCgiBodyToWrite().empty()) {
        removeFromEpoll(fd);
        _cgiWriteFds.erase(fd);
        client->getCgi().closeWritePipe();
    }
}

// ─── CGI read (CGI stdout -> server) ────────────────────────────────────────

void Server::readCgi(int fd) {
    Client *client = _cgiReadFds[fd];
    if (!client) { removeFromEpoll(fd); close(fd); return; }
    client->updateActivity();

    // One read per readiness notification (level-triggered EPOLLIN). While
    // the CGI keeps producing output, epoll re-fires; when the process closes
    // its stdout we get EOF (read returns 0). No errno inspection: a
    // non-positive return marks the end of the CGI output.
    char buf[READ_BUFSIZE];
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        client->getCgiOutput().append(buf, static_cast<size_t>(n));
        return;
    }

    // n == 0 (EOF) or n < 0 (error): CGI output complete — process it.
    removeFromEpoll(fd);
    _cgiReadFds.erase(fd);
    client->getCgi().closePipes();

    // Stdout EOF means the CGI process is done (it was the only writer of
    // that pipe's write end). Reap it now and check how it exited: a
    // non-zero exit / crash produced no valid CGI output, so surface a 500
    // instead of silently returning 200 with an empty body.
    bool cgiFailed = false;
    pid_t cgiPid = client->getCgi().getPid();
    if (cgiPid > 0) {
        int status = 0;
        if (waitpid(cgiPid, &status, 0) > 0)
            cgiFailed = !WIFEXITED(status) || WEXITSTATUS(status) != 0;
    }

    finishCgi(client, cgiFailed);
}

void Server::finishCgi(Client *client, bool cgiFailed) {
    int statusCode;
    std::map<std::string, std::string> cgiHeaders;
    std::string cgiBody;

    CgiHandler::parseCgiOutput(client->getCgiOutput(), statusCode, cgiHeaders, cgiBody);

    if (cgiFailed) {
        statusCode = 500;
        cgiHeaders.clear();
        cgiBody = HttpResponse::buildErrorPage(500);
    }

    std::ostringstream resp;
    resp << "HTTP/1.1 " << statusCode << " "
         << HttpResponse::statusMessage(statusCode) << "\r\n";
    resp << "Date: " << getCurrentDate() << "\r\n";
    resp << "Server: webserv/1.0\r\n";

    for (std::map<std::string, std::string>::const_iterator it = cgiHeaders.begin();
         it != cgiHeaders.end(); ++it) {
        resp << it->first << ": " << it->second << "\r\n";
    }

    if (cgiHeaders.find("Content-Length") == cgiHeaders.end() &&
        cgiHeaders.find("content-length") == cgiHeaders.end())
        resp << "Content-Length: " << cgiBody.size() << "\r\n";

    if (client->isKeepAlive())
        resp << "Connection: keep-alive\r\n";
    else
        resp << "Connection: close\r\n";

    if (client->isSessionNew())
        resp << setCookieHeaderLine(client);

    resp << "\r\n";
    resp << cgiBody;

    client->setSendBuffer(resp.str());
    client->setState(CLIENT_SENDING);
    modEpoll(client->getFd(), EPOLLOUT);
}

// ─── timeouts ───────────────────────────────────────────────────────────────

void Server::checkTimeouts() {
    time_t now = time(NULL);
    std::vector<int> toClose;

    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (now - it->second->getLastActivity() > TIMEOUT_SEC)
            toClose.push_back(it->first);
    }
    for (size_t i = 0; i < toClose.size(); ++i)
        closeClient(toClose[i]);

    _sessionMgr.cleanupExpired();
}

// ─── close ──────────────────────────────────────────────────────────────────

void Server::closeClient(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it == _clients.end()) return;

    Client *client = it->second;

    // If a CGI is still running for this client (e.g. it hung and we're
    // closing on timeout), it will never finish on its own — kill it so it
    // doesn't linger as an orphan process. The exit is reaped by the
    // waitpid(-1, ..., WNOHANG) sweep in run().
    if (client->getState() == CLIENT_CGI_WRITING || client->getState() == CLIENT_CGI_READING) {
        pid_t cgiPid = client->getCgi().getPid();
        if (cgiPid > 0)
            kill(cgiPid, SIGKILL);
    }

    // Clean up CGI pipe fds
    int cgiWrite = client->getCgi().getPipeInWrite();
    int cgiRead  = client->getCgi().getPipeOutRead();

    if (cgiWrite >= 0) {
        removeFromEpoll(cgiWrite);
        _cgiWriteFds.erase(cgiWrite);
    }
    if (cgiRead >= 0) {
        removeFromEpoll(cgiRead);
        _cgiReadFds.erase(cgiRead);
    }
    client->getCgi().closePipes();

    removeFromEpoll(fd);
    close(fd);
    delete client;
    _clients.erase(it);
}

// ─── server/location matching ───────────────────────────────────────────────

const ServerConfig *Server::findServer(int listenFd, const std::string &host) const {
    std::map<int, size_t>::const_iterator lit = _listenFds.find(listenFd);
    if (lit == _listenFds.end()) return NULL;

    size_t defaultIdx = lit->second;
    const ServerConfig *def = &_servers[defaultIdx];

    // Strip port from host header
    std::string hostName = host;
    size_t colon = hostName.rfind(':');
    if (colon != std::string::npos) hostName = hostName.substr(0, colon);

    if (hostName.empty()) return def;

    // Find matching server by server_name on the same port
    int port = _servers[defaultIdx].port;
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].port != port) continue;
        const std::vector<std::string> &names = _servers[i].serverNames;
        for (size_t j = 0; j < names.size(); ++j) {
            if (names[j] == hostName)
                return &_servers[i];
        }
    }
    return def;
}

const LocationConfig *Server::findLocation(const ServerConfig &srv, const std::string &path) const {
    const LocationConfig *best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < srv.locations.size(); ++i) {
        const std::string &locPath = srv.locations[i].path;
        if (startsWith(path, locPath)) {
            // Ensure it's a proper prefix boundary
            if (locPath.size() > bestLen &&
                (locPath == "/" || path.size() == locPath.size() ||
                 path[locPath.size()] == '/' || locPath[locPath.size()-1] == '/')) {
                bestLen = locPath.size();
                best = &srv.locations[i];
            }
        }
    }
    return best;
}

std::string Server::resolvePath(const LocationConfig &loc, const std::string &uriPath) const {
    // "alias"-style semantics, matching the subject's own example verbatim:
    // location /kapouet, root /tmp/www, URI /kapouet/pouic/toto/pouet ->
    // /tmp/www/pouic/toto/pouet. The location prefix is stripped from the
    // URI before the remainder is appended to root.
    std::string root = loc.root;
    // Strip trailing slash from root (except bare ".")
    if (root.size() > 1 && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);

    // Ensure uriPath starts with /
    std::string uri = uriPath;
    if (uri.empty() || uri[0] != '/') uri = "/" + uri;

    std::string remainder = uri;
    if (uri.compare(0, loc.path.size(), loc.path) == 0)
        remainder = uri.substr(loc.path.size());
    if (remainder.empty() || remainder[0] != '/')
        remainder = "/" + remainder;

    return root + remainder;
}

bool Server::methodAllowed(const LocationConfig &loc, const std::string &method) const {
    if (loc.methods.empty()) return true; // no restriction
    for (size_t i = 0; i < loc.methods.size(); ++i) {
        if (loc.methods[i] == method) return true;
    }
    return false;
}

bool Server::isCgiRequest(const LocationConfig &loc, const std::string &path,
                           std::string &interpreter) const {
    std::string ext = getExtension(path);
    if (ext.empty()) return false;
    std::map<std::string, std::string>::const_iterator it = loc.cgiExtensions.find(ext);
    if (it == loc.cgiExtensions.end()) return false;
    interpreter = it->second;
    return true;
}

// ─── error response ─────────────────────────────────────────────────────────

std::string Server::makeErrorResponse(Client *client, int code, const ServerConfig *srv) {
    std::string customBody;
    if (srv) {
        std::map<int, std::string>::const_iterator it = srv->errorPages.find(code);
        if (it != srv->errorPages.end()) {
            // Resolve relative to first location root or cwd
            std::string errPath = it->second;
            if (!errPath.empty() && errPath[0] == '/') {
                // Try www + path
                std::string fullPath = joinPath("./www", errPath);
                if (isRegularFile(fullPath))
                    customBody = readFile(fullPath);
            }
            if (customBody.empty() && isRegularFile(errPath))
                customBody = readFile(errPath);
        }
    }

    HttpResponse resp;
    std::string connHdr = client->isKeepAlive() ? "keep-alive" : "close";
    resp.setHeader("Connection", connHdr);
    std::string body = customBody.empty() ? HttpResponse::buildErrorPage(code) : customBody;
    return resp.buildError(code, body);
}

// ─── process request ────────────────────────────────────────────────────────

void Server::processRequest(Client *client) {
    const HttpRequest &req = client->getRequest();

    // Find listen fd for this client to look up server config
    // We do this by finding which server group this client belongs to.
    // Since we only have _clients mapped by fd, we need to find a listen fd
    // on the same port. We'll just use the first server for now and rely on Host:.
    // Better: track which listen fd accepted this client.
    // For simplicity, we find servers matching Host header.

    std::string host = req.getHeader("host");
    int listenFd = client->getListenFd();
    const ServerConfig *srv = findServer(listenFd, host);
    if (!srv) {
        // Fallback: should not happen, but be safe
        if (!_servers.empty()) srv = &_servers[0];
        else { closeClient(client->getFd()); return; }
    }

    client->setKeepAlive(req.isKeepAlive());

    // Re-enforce max body size now that we have a server (and, if the path
    // matches one, a location — a location's own client_max_body_size, when
    // set, overrides the server-wide default).
    const LocationConfig *bodyLoc = findLocation(*srv, req.getPath());
    size_t maxBody = (bodyLoc && bodyLoc->clientMaxBodySize > 0)
        ? bodyLoc->clientMaxBodySize : srv->clientMaxBodySize;
    if (req.getBody().size() > maxBody) {
        std::string resp = makeErrorResponse(client, 413, srv);
        client->setSendBuffer(resp);
        client->setState(CLIENT_SENDING);
        client->setKeepAlive(false);
        modEpoll(client->getFd(), EPOLLOUT);
        return;
    }

    // Session (bonus): resolve/create before routing so a built-in route
    // (e.g. /session) or a CGI script (via HTTP_COOKIE) can see it, and so
    // we know whether this response needs to hand out a fresh cookie.
    bool isNewSession = false;
    SessionData &sess = _sessionMgr.getOrCreate(req.getHeader("cookie"), isNewSession);
    client->setSessionInfo(sess.id, isNewSession);

    std::string response = handleRequest(client, *srv);

    // Check if client was closed during handleRequest (shouldn't happen but be safe)
    if (_clients.find(client->getFd()) == _clients.end()) return;

    if (client->getState() == CLIENT_CGI_WRITING ||
        client->getState() == CLIENT_CGI_READING) {
        return; // CGI in progress — finishCgi() attaches the cookie later
    }

    if (client->isSessionNew()) {
        size_t sep = response.find("\r\n\r\n");
        if (sep != std::string::npos)
            response.insert(sep + 2, setCookieHeaderLine(client));
    }

    client->setSendBuffer(response);
    client->setState(CLIENT_SENDING);
    modEpoll(client->getFd(), EPOLLOUT);
}

std::string Server::setCookieHeaderLine(Client *client) const {
    return "Set-Cookie: " SESSION_COOKIE_NAME "=" + client->getSessionId() +
           "; Path=/; HttpOnly; Max-Age=" + intToString(SESSION_TIMEOUT_SEC) + "\r\n";
}

// ─── handle request ─────────────────────────────────────────────────────────

std::string Server::handleRequest(Client *client, const ServerConfig &srv) {
    const HttpRequest &req = client->getRequest();
    const std::string &path = req.getPath();

    // Built-in session/cookie demo (bonus) — no config or CGI needed.
    if (path == "/session") {
        return handleSessionDemo(client);
    }

    // Prevent path traversal
    if (path.find("..") != std::string::npos) {
        return makeErrorResponse(client, 403, &srv);
    }

    // A location configured with a trailing slash (e.g. "/directory/") is a
    // directory mount — requesting it without the slash ("/directory")
    // shouldn't silently fall through to whatever *other* location happens
    // to match instead (usually "/"). Redirect to the slash form, matching
    // standard nginx/Apache behavior for directory locations.
    for (size_t i = 0; i < srv.locations.size(); ++i) {
        const std::string &lp = srv.locations[i].path;
        if (lp.size() > 1 && lp[lp.size() - 1] == '/' && lp.substr(0, lp.size() - 1) == path) {
            HttpResponse resp;
            resp.setStatus(301);
            resp.setHeader("Location", path + "/");
            resp.setHeader("Content-Length", "0");
            resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
            return resp.build();
        }
    }

    const LocationConfig *loc = findLocation(srv, path);
    if (!loc) {
        return makeErrorResponse(client, 404, &srv);
    }

    // Redirect
    if (!loc->redirect.empty()) {
        HttpResponse resp;
        resp.setStatus(loc->redirectCode > 0 ? loc->redirectCode : 302);
        resp.setHeader("Location", loc->redirect);
        resp.setHeader("Content-Length", "0");
        resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
        return resp.build();
    }

    // Method check: matched strictly against the configured list — a
    // location allowing GET does not implicitly allow HEAD too. If you
    // want HEAD supported on a route, list it explicitly in `methods`.
    if (!methodAllowed(*loc, req.getMethod())) {
        return makeErrorResponse(client, 405, &srv);
    }

    // Route by method
    if (req.getMethod() == "GET" || req.getMethod() == "HEAD") {
        return handleGet(client, srv, *loc);
    } else if (req.getMethod() == "POST") {
        return handlePost(client, srv, *loc);
    } else if (req.getMethod() == "DELETE") {
        return handleDelete(client, srv, *loc);
    }

    return makeErrorResponse(client, 501, &srv);
}

// ─── session / cookies demo (bonus) ─────────────────────────────────────────

std::string Server::handleSessionDemo(Client *client) {
    SessionData *sess = _sessionMgr.find(client->getSessionId());
    if (sess) sess->visits++;

    std::ostringstream body;
    body << "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
            "<title>Session Demo</title>"
            "<style>body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;"
            "display:flex;flex-direction:column;align-items:center;justify-content:center;"
            "height:100vh;margin:0;text-align:center;}"
            "h1{color:#7b8cde;}code{color:#68d391;}a{color:#7b8cde;}</style>"
            "</head><body>"
            "<h1>Session &amp; Cookies Demo</h1>"
         << "<p>Session ID: <code>" << (sess ? sess->id.substr(0, 8) : "?") << "&hellip;</code></p>"
         << "<p>Visits this session: <code>" << (sess ? sess->visits : 0) << "</code></p>"
         << "<p>" << (client->isSessionNew()
                       ? "New session just created &mdash; reload to watch the count go up."
                       : "Existing session recognized from the cookie.")
         << "</p>"
            "<a href=\"/\">Go Home</a></body></html>";

    HttpResponse resp;
    resp.setStatus(200);
    resp.setHeader("Content-Type", "text/html");
    resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
    resp.setBody(body.str());
    return resp.build();
}

// ─── GET ────────────────────────────────────────────────────────────────────

std::string Server::handleGet(Client *client, const ServerConfig &srv, const LocationConfig &loc) {
    const HttpRequest &req = client->getRequest();
    std::string filePath = resolvePath(loc, req.getPath());

    // CGI check
    std::string interpreter;
    if (isCgiRequest(loc, filePath, interpreter)) {
        if (!isRegularFile(filePath))
            return makeErrorResponse(client, 404, &srv);
        startCgiForClient(client, srv, loc, filePath, interpreter);
        return "";
    }

    // Directory
    if (isDirectory(filePath)) {
        // Try index file
        if (!loc.index.empty()) {
            std::string indexPath = joinPath(filePath, loc.index);
            if (isRegularFile(indexPath)) {
                int code = 200;
                std::string body = serveFile(indexPath, code);
                HttpResponse resp;
                resp.setStatus(code);
                resp.setHeader("Content-Type", getMimeType(indexPath));
                resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
                if (req.getMethod() == "HEAD") {
                    resp.setHeader("Content-Length", sizeToString(body.size()));
                    resp.setBody("");
                } else {
                    resp.setBody(body);
                }
                return resp.build();
            }
        }
        // Autoindex
        if (loc.autoindex) {
            std::string listing = generateDirectoryListing(filePath, req.getPath());
            if (listing.empty())
                return makeErrorResponse(client, 403, &srv);
            HttpResponse resp;
            resp.setStatus(200);
            resp.setHeader("Content-Type", "text/html");
            resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
            if (req.getMethod() == "HEAD") {
                resp.setHeader("Content-Length", sizeToString(listing.size()));
                resp.setBody("");
            } else {
                resp.setBody(listing);
            }
            return resp.build();
        }
        // No index file present and autoindex is off: nothing here to
        // serve, and not distinguishable from "this doesn't exist" from
        // the client's point of view.
        return makeErrorResponse(client, 404, &srv);
    }

    // Regular file
    if (!isRegularFile(filePath))
        return makeErrorResponse(client, 404, &srv);

    int code = 200;
    std::string body = serveFile(filePath, code);
    if (code != 200)
        return makeErrorResponse(client, code, &srv);

    HttpResponse resp;
    resp.setStatus(200);
    resp.setHeader("Content-Type", getMimeType(filePath));
    resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
    if (req.getMethod() == "HEAD") {
        resp.setHeader("Content-Length", sizeToString(body.size()));
        resp.setBody("");
    } else {
        resp.setBody(body);
    }
    return resp.build();
}

std::string Server::serveFile(const std::string &path, int &statusCode) {
    std::string content = readFile(path);
    if (content.empty() && getFileSize(path) > 0) {
        statusCode = 403;
        return "";
    }
    statusCode = 200;
    return content;
}

// ─── POST ───────────────────────────────────────────────────────────────────

std::string Server::handlePost(Client *client, const ServerConfig &srv, const LocationConfig &loc) {
    const HttpRequest &req = client->getRequest();
    std::string filePath = resolvePath(loc, req.getPath());

    // CGI check first
    std::string interpreter;
    if (isCgiRequest(loc, filePath, interpreter)) {
        if (!isRegularFile(filePath))
            return makeErrorResponse(client, 404, &srv);
        startCgiForClient(client, srv, loc, filePath, interpreter);
        return "";
    }

    // Upload?
    if (!loc.uploadPath.empty()) {
        return handleUpload(client, srv, loc);
    }

    return makeErrorResponse(client, 405, &srv);
}

// ─── upload ─────────────────────────────────────────────────────────────────

static std::string extractBoundary(const std::string &contentType) {
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos) return "";
    std::string boundary = contentType.substr(pos + 9);
    // Strip optional quotes
    if (!boundary.empty() && boundary[0] == '"') {
        boundary = boundary.substr(1, boundary.rfind('"') - 1);
    }
    return boundary;
}

static std::string extractFilename(const std::string &disposition) {
    size_t pos = disposition.find("filename=\"");
    if (pos == std::string::npos) return "";
    pos += 10;
    size_t end = disposition.find('"', pos);
    if (end == std::string::npos) return "";
    return disposition.substr(pos, end - pos);
}

std::string Server::handleUpload(Client *client, const ServerConfig &srv, const LocationConfig &loc) {
    const HttpRequest &req = client->getRequest();
    const std::string &body = req.getBody();
    std::string ct = req.getHeader("content-type");
    std::string uploadDir = loc.uploadPath;

    if (uploadDir.empty())
        return makeErrorResponse(client, 500, &srv);

    // The upload directory is provided via the configuration (upload_path)
    // and is expected to already exist; if it does not, the ofstream open
    // below fails and we return 500.

    bool uploaded = false;
    std::string uploadedName;

    if (!ct.empty() && ct.find("multipart/form-data") != std::string::npos) {
        std::string boundary = extractBoundary(ct);
        if (boundary.empty())
            return makeErrorResponse(client, 400, &srv);

        std::string delimiter = "--" + boundary;
        std::string endDelimiter = "--" + boundary + "--";

        size_t pos = 0;
        while (pos < body.size()) {
            size_t start = body.find(delimiter, pos);
            if (start == std::string::npos) break;
            start += delimiter.size();

            // Skip \r\n after boundary
            if (start < body.size() && body[start] == '\r') start++;
            if (start < body.size() && body[start] == '\n') start++;

            // Check for end boundary
            if (startsWith(body.substr(start > 2 ? start - 2 : 0), "--"))
                break;
            if (body.substr(start, 2) == "--") break;

            // Find end of this part
            size_t partEnd = body.find("\r\n" + delimiter, start);
            if (partEnd == std::string::npos) {
                partEnd = body.find("\n" + delimiter, start);
                if (partEnd == std::string::npos) break;
            }

            std::string part = body.substr(start, partEnd - start);
            pos = partEnd + delimiter.size() + 2;

            // Parse part headers
            size_t headerEnd = part.find("\r\n\r\n");
            size_t bodyStart = 4;
            if (headerEnd == std::string::npos) {
                headerEnd = part.find("\n\n");
                bodyStart = 2;
                if (headerEnd == std::string::npos) continue;
            }

            std::string partHeaders = part.substr(0, headerEnd);
            std::string partBody    = part.substr(headerEnd + bodyStart);

            // Extract Content-Disposition
            std::string disposition;
            std::istringstream hs(partHeaders);
            std::string hline;
            while (std::getline(hs, hline)) {
                if (!hline.empty() && hline[hline.size()-1] == '\r')
                    hline.erase(hline.size()-1);
                if (toLower(hline.substr(0, 20)) == "content-disposition:")
                    disposition = trim(hline.substr(20));
            }

            std::string filename = extractFilename(disposition);
            if (filename.empty()) continue;

            // Sanitize filename
            for (size_t i = 0; i < filename.size(); ++i) {
                if (filename[i] == '/' || filename[i] == '\\' || filename[i] == '\0')
                    filename[i] = '_';
            }

            std::string outPath = joinPath(uploadDir, filename);
            std::ofstream out(outPath.c_str(), std::ios::binary);
            if (!out.is_open())
                return makeErrorResponse(client, 500, &srv);
            out.write(partBody.c_str(), static_cast<std::streamsize>(partBody.size()));
            out.close();
            uploaded = true;
            uploadedName = filename;
        }
    } else {
        // Raw body upload - generate a filename
        time_t now = time(NULL);
        std::ostringstream fname;
        fname << "upload_" << static_cast<long>(now);
        std::string filename = fname.str();
        std::string outPath = joinPath(uploadDir, filename);
        std::ofstream out(outPath.c_str(), std::ios::binary);
        if (!out.is_open())
            return makeErrorResponse(client, 500, &srv);
        out.write(body.c_str(), static_cast<std::streamsize>(body.size()));
        out.close();
        uploaded = true;
        uploadedName = filename;
    }

    if (!uploaded)
        return makeErrorResponse(client, 400, &srv);

    std::string respBody = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<title>Upload Successful</title>"
        "<style>body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;"
        "display:flex;flex-direction:column;align-items:center;justify-content:center;"
        "height:100vh;margin:0;}h1{color:#68d391;}a{color:#7b8cde;}</style></head>"
        "<body><h1>Upload Successful</h1><p>File <strong>" + uploadedName +
        "</strong> uploaded.</p><a href=\"/\">Go Home</a></body></html>";

    HttpResponse resp;
    resp.setStatus(201);
    resp.setHeader("Content-Type", "text/html");
    resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
    resp.setBody(respBody);
    return resp.build();
}

// ─── DELETE ─────────────────────────────────────────────────────────────────

std::string Server::handleDelete(Client *client, const ServerConfig &srv, const LocationConfig &loc) {
    const HttpRequest &req = client->getRequest();
    std::string filePath = resolvePath(loc, req.getPath());

    if (!fileExists(filePath))
        return makeErrorResponse(client, 404, &srv);

    if (isDirectory(filePath))
        return makeErrorResponse(client, 403, &srv);

    if (!isRegularFile(filePath))
        return makeErrorResponse(client, 403, &srv);

    if (remove(filePath.c_str()) != 0) {
        if (errno == EACCES || errno == EPERM)
            return makeErrorResponse(client, 403, &srv);
        return makeErrorResponse(client, 500, &srv);
    }

    std::string body = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<title>Deleted</title>"
        "<style>body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;"
        "display:flex;flex-direction:column;align-items:center;justify-content:center;"
        "height:100vh;margin:0;}h1{color:#fc8181;}a{color:#7b8cde;}</style></head>"
        "<body><h1>File Deleted</h1><p>" + req.getPath() + " has been removed.</p>"
        "<a href=\"/\">Go Home</a></body></html>";

    HttpResponse resp;
    resp.setStatus(200);
    resp.setHeader("Content-Type", "text/html");
    resp.setHeader("Connection", client->isKeepAlive() ? "keep-alive" : "close");
    resp.setBody(body);
    return resp.build();
}

// ─── CGI ────────────────────────────────────────────────────────────────────

void Server::startCgiForClient(Client *client, const ServerConfig &srv,
                                const LocationConfig &loc,
                                const std::string &scriptPath,
                                const std::string &interpreter) {
    CgiHandler &cgi = client->getCgi();

    if (!cgi.setup(client->getRequest(), loc, srv, scriptPath, interpreter)) {
        std::string resp = makeErrorResponse(client, 500, &srv);
        client->setSendBuffer(resp);
        client->setState(CLIENT_SENDING);
        modEpoll(client->getFd(), EPOLLOUT);
        return;
    }

    // Set up CGI body to write
    const std::string &body = client->getRequest().getBody();
    client->setCgiBodyToWrite(body);
    client->getCgiOutput().clear();

    int writefd = cgi.getPipeInWrite();
    int readfd  = cgi.getPipeOutRead();

    if (!body.empty() && writefd >= 0) {
        client->setState(CLIENT_CGI_WRITING);
        _cgiWriteFds[writefd] = client;
        addToEpoll(writefd, EPOLLOUT);
    } else {
        // No body to write — close write pipe immediately
        cgi.closeWritePipe();
    }

    if (readfd >= 0) {
        _cgiReadFds[readfd] = client;
        addToEpoll(readfd, EPOLLIN);
        client->setState(CLIENT_CGI_READING);
    }
}
