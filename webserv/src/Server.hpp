#ifndef SERVER_HPP
#define SERVER_HPP

#include <map>
#include <vector>
#include <string>
#include <sys/epoll.h>
#include "Config.hpp"
#include "Client.hpp"
#include "SessionManager.hpp"

#define MAX_EVENTS   1024
#define TIMEOUT_SEC  30
#define READ_BUFSIZE 65536

class Server {
public:
    Server();
    ~Server();

    bool init(const Config &cfg);
    void run();

private:
    int                        _epollFd;
    std::vector<ServerConfig>  _servers;
    size_t                     _maxBodyCeiling;

    // listen fd -> ServerConfig index
    std::map<int, size_t>      _listenFds;

    // client fd -> Client*
    std::map<int, Client*>     _clients;

    // cgi write pipe fd -> Client*
    std::map<int, Client*>     _cgiWriteFds;
    // cgi read pipe fd -> Client*
    std::map<int, Client*>     _cgiReadFds;

    SessionManager             _sessionMgr;

    bool addToEpoll(int fd, uint32_t events);
    bool modEpoll(int fd, uint32_t events);
    void removeFromEpoll(int fd);

    void acceptClient(int listenFd);
    void feedAndProcess(Client *client, const char *data, size_t len);
    void readClient(int fd);
    void writeClient(int fd);
    void writeCgi(int fd);
    void readCgi(int fd);
    void checkTimeouts();
    void closeClient(int fd);

    // Request processing
    void processRequest(Client *client);
    std::string handleRequest(Client *client, const ServerConfig &srv);
    std::string handleGet(Client *client, const ServerConfig &srv, const LocationConfig &loc);
    std::string handlePost(Client *client, const ServerConfig &srv, const LocationConfig &loc);
    std::string handleDelete(Client *client, const ServerConfig &srv, const LocationConfig &loc);
    std::string handleCgi(Client *client, const ServerConfig &srv, const LocationConfig &loc,
                          const std::string &scriptPath, const std::string &interpreter);
    std::string serveFile(const std::string &path, int &statusCode);
    std::string makeErrorResponse(Client *client, int code, const ServerConfig *srv);
    std::string handleUpload(Client *client, const ServerConfig &srv, const LocationConfig &loc);
    std::string handleSessionDemo(Client *client);
    std::string setCookieHeaderLine(Client *client) const;

    const ServerConfig *findServer(int listenFd, const std::string &host) const;
    const LocationConfig *findLocation(const ServerConfig &srv, const std::string &path) const;
    std::string resolvePath(const LocationConfig &loc, const std::string &uriPath) const;

    bool methodAllowed(const LocationConfig &loc, const std::string &method) const;
    bool isCgiRequest(const LocationConfig &loc, const std::string &path,
                      std::string &interpreter) const;
    void startCgiForClient(Client *client, const ServerConfig &srv,
                           const LocationConfig &loc,
                           const std::string &scriptPath,
                           const std::string &interpreter);
    void finishCgi(Client *client, bool cgiFailed);
};

#endif
