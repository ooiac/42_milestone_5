#include "CgiHandler.hpp"
#include "Utils.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>

CgiHandler::CgiHandler() : _pid(-1), _hasBody(false) {
    _pipeIn[0]  = -1;
    _pipeIn[1]  = -1;
    _pipeOut[0] = -1;
    _pipeOut[1] = -1;
}

CgiHandler::~CgiHandler() {}

int   CgiHandler::getPipeInWrite() const  { return _pipeIn[1]; }
int   CgiHandler::getPipeOutRead() const  { return _pipeOut[0]; }
pid_t CgiHandler::getPid() const          { return _pid; }
bool  CgiHandler::hasBody() const         { return _hasBody; }

static void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

char **CgiHandler::makeEnvArray() {
    char **env = new char*[_env.size() + 1];
    for (size_t i = 0; i < _env.size(); ++i) {
        env[i] = new char[_env[i].size() + 1];
        memcpy(env[i], _env[i].c_str(), _env[i].size() + 1);
    }
    env[_env.size()] = NULL;
    return env;
}

void CgiHandler::freeEnvArray(char **env) {
    for (size_t i = 0; env[i] != NULL; ++i)
        delete[] env[i];
    delete[] env;
}

bool CgiHandler::setup(const HttpRequest &req,
                       const LocationConfig &loc,
                       const ServerConfig &srv,
                       const std::string &scriptPathIn,
                       const std::string &interpreter) {
    // Split the script path into its directory and basename. The child
    // chdir()s into the directory and execs the script by basename, so the
    // CGI runs in the correct directory for relative file access without
    // needing realpath() (which is not in the allowed function list).
    std::string scriptPath = scriptPathIn;
    std::string scriptDir  = ".";
    std::string scriptName = scriptPath;
    {
        size_t slash = scriptPath.rfind('/');
        if (slash != std::string::npos) {
            scriptDir  = scriptPath.substr(0, slash);
            scriptName = scriptPath.substr(slash + 1);
            if (scriptDir.empty()) scriptDir = "/";
        }
    }

    _hasBody = !req.getBody().empty();

    // Build environment
    _env.clear();
    _env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _env.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _env.push_back("SERVER_SOFTWARE=webserv/1.0");
    _env.push_back("REQUEST_METHOD=" + req.getMethod());
    _env.push_back("QUERY_STRING=" + req.getQueryString());
    _env.push_back("SCRIPT_FILENAME=" + scriptPath);
    _env.push_back("SCRIPT_NAME=" + req.getPath());
    // This server only dispatches to CGI when the resolved path is exactly
    // an existing script file, so there is never any extra path info beyond
    // the script itself — PATH_INFO mirrors the request path, matching
    // REQUEST_URI (also the request path here, since query string is
    // reported separately via QUERY_STRING).
    _env.push_back("PATH_INFO=" + req.getPath());
    _env.push_back("REQUEST_URI=" + req.getUri());
    _env.push_back("PATH_TRANSLATED=" + scriptPath);
    _env.push_back("SERVER_NAME=" + (srv.serverNames.empty() ? srv.host : srv.serverNames[0]));
    _env.push_back("SERVER_PORT=" + intToString(srv.port));
    _env.push_back("REDIRECT_STATUS=200");
    _env.push_back("DOCUMENT_ROOT=" + loc.root);

    std::string ct = req.getHeader("content-type");
    if (!ct.empty())
        _env.push_back("CONTENT_TYPE=" + ct);
    else
        _env.push_back("CONTENT_TYPE=");

    if (_hasBody)
        _env.push_back("CONTENT_LENGTH=" + sizeToString(req.getBody().size()));
    else
        _env.push_back("CONTENT_LENGTH=0");

    // HTTP_* headers
    const std::map<std::string, std::string> &hdrs = req.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = hdrs.begin();
         it != hdrs.end(); ++it) {
        std::string key = "HTTP_" + toUpper(it->first);
        // Replace - with _
        for (size_t i = 0; i < key.size(); ++i)
            if (key[i] == '-') key[i] = '_';
        _env.push_back(key + "=" + it->second);
    }

    // Create pipes
    if (pipe(_pipeIn) < 0 || pipe(_pipeOut) < 0) {
        std::cerr << "CgiHandler: pipe() failed" << std::endl;
        return false;
    }

    // Make pipe ends non-blocking (used by epoll in parent)
    setNonBlocking(_pipeIn[1]);   // write end: parent writes body
    setNonBlocking(_pipeOut[0]);  // read end: parent reads response

    _pid = fork();
    if (_pid < 0) {
        std::cerr << "CgiHandler: fork() failed" << std::endl;
        close(_pipeIn[0]);  close(_pipeIn[1]);
        close(_pipeOut[0]); close(_pipeOut[1]);
        return false;
    }

    if (_pid == 0) {
        // Child process
        // stdin  <- pipeIn[0]
        // stdout -> pipeOut[1]
        if (dup2(_pipeIn[0], STDIN_FILENO) < 0)  _exit(1);
        if (dup2(_pipeOut[1], STDOUT_FILENO) < 0) _exit(1);

        close(_pipeIn[0]);
        close(_pipeIn[1]);
        close(_pipeOut[0]);
        close(_pipeOut[1]);

        // Change working directory to the script's directory, then exec the
        // script by basename so relative paths inside the CGI resolve there.
        if (chdir(scriptDir.c_str()) != 0)
            _exit(1);

        char **env = makeEnvArray();

        char *args[3];
        char *interpCopy = new char[interpreter.size() + 1];
        memcpy(interpCopy, interpreter.c_str(), interpreter.size() + 1);
        char *scriptCopy = new char[scriptName.size() + 1];
        memcpy(scriptCopy, scriptName.c_str(), scriptName.size() + 1);
        args[0] = interpCopy;
        args[1] = scriptCopy;
        args[2] = NULL;

        execve(interpreter.c_str(), args, env);
        // If execve fails
        std::cerr << "CGI execve failed: " << interpreter << std::endl;
        _exit(1);
    }

    // Parent: close ends used by child
    close(_pipeIn[0]);
    close(_pipeOut[1]);

    return true;
}

void CgiHandler::closeWritePipe() {
    if (_pipeIn[1] >= 0) {
        close(_pipeIn[1]);
        _pipeIn[1] = -1;
    }
}

void CgiHandler::closePipes() {
    if (_pipeIn[1] >= 0)  { close(_pipeIn[1]);  _pipeIn[1]  = -1; }
    if (_pipeOut[0] >= 0) { close(_pipeOut[0]); _pipeOut[0] = -1; }
}

bool CgiHandler::parseCgiOutput(const std::string &raw,
                                 int &statusCode,
                                 std::map<std::string, std::string> &headers,
                                 std::string &body) {
    statusCode = 200;
    headers.clear();
    body.clear();

    // Find header/body separator
    size_t sep = raw.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == std::string::npos) {
        sep = raw.find("\n\n");
        sepLen = 2;
    }
    if (sep == std::string::npos) {
        // No headers, treat as body
        body = raw;
        return true;
    }

    std::string headerSection = raw.substr(0, sep);
    body = raw.substr(sep + sepLen);

    // Parse headers
    std::istringstream ss(headerSection);
    std::string line;
    while (std::getline(ss, line)) {
        // Remove \r
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string name  = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));

        // Status header
        if (toLower(name) == "status") {
            statusCode = atoi(value.c_str());
        } else {
            headers[name] = value;
        }
    }

    return true;
}
