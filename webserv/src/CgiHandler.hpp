#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <string>
#include <map>
#include <vector>
#include <sys/types.h>
#include "HttpRequest.hpp"
#include "Config.hpp"

class CgiHandler {
public:
    CgiHandler();
    ~CgiHandler();

    bool setup(const HttpRequest &req,
               const LocationConfig &loc,
               const ServerConfig &srv,
               const std::string &scriptPath,
               const std::string &interpreter);

    // Returns pipe fds for epoll integration
    int  getPipeInWrite() const;   // fd to write request body to CGI stdin
    int  getPipeOutRead() const;   // fd to read CGI stdout
    pid_t getPid() const;

    // If body is empty, no pipe_in needed
    bool hasBody() const;

    // After CGI finishes, parse output
    static bool parseCgiOutput(const std::string &raw,
                                int &statusCode,
                                std::map<std::string, std::string> &headers,
                                std::string &body);

    void closePipes();
    void closeWritePipe();

private:
    pid_t  _pid;
    int    _pipeIn[2];
    int    _pipeOut[2];
    bool   _hasBody;

    std::vector<std::string> _env;
    char **makeEnvArray();
    void   freeEnvArray(char **env);
};

#endif
