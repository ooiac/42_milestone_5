#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "CgiHandler.hpp"
#include "Config.hpp"

enum ClientState {
    CLIENT_READING,
    CLIENT_SENDING,
    CLIENT_CGI_WRITING,
    CLIENT_CGI_READING,
    CLIENT_DONE
};

class Client {
public:
    explicit Client(int fd, const std::string &clientIp, int listenFd = -1);
    ~Client();

    int         getFd() const;
    ClientState getState() const;
    void        setState(ClientState s);
    bool        isKeepAlive() const;
    void        setKeepAlive(bool v);
    time_t      getLastActivity() const;
    void        updateActivity();

    // Request parsing
    HttpRequest  &getRequest();

    // Response building
    HttpResponse &getResponse();

    // Send buffer
    const std::string &getSendBuffer() const;
    void               setSendBuffer(const std::string &buf);
    void               consumeSendBuffer(size_t n);
    bool               hasPendingSend() const;

    // CGI
    CgiHandler  &getCgi();
    void         setCgiBodyToWrite(const std::string &body);
    const std::string &getCgiBodyToWrite() const;
    void               consumeCgiBody(size_t n);

    std::string &getCgiOutput();

    const std::string &getClientIp() const;
    int                getListenFd() const;

    // Session (bonus): resolved once per request in Server::processRequest,
    // then consulted wherever the response for that same request is
    // finished (immediately for static/CGI-less paths, later in
    // Server::finishCgi() for CGI ones).
    void               setSessionInfo(const std::string &id, bool isNew);
    const std::string &getSessionId() const;
    bool               isSessionNew() const;

private:
    int          _fd;
    int          _listenFd;
    ClientState  _state;
    bool         _keepAlive;
    time_t       _lastActivity;
    std::string  _clientIp;

    HttpRequest   _request;
    HttpResponse  _response;
    CgiHandler    _cgi;

    std::string   _sendBuffer;
    std::string   _cgiBodyToWrite;
    std::string   _cgiOutput;

    std::string   _sessionId;
    bool          _sessionIsNew;
};

#endif
