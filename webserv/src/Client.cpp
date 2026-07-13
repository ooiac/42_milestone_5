#include "Client.hpp"

Client::Client(int fd, const std::string &clientIp, int listenFd)
    : _fd(fd), _listenFd(listenFd), _state(CLIENT_READING), _keepAlive(false),
      _lastActivity(time(NULL)), _clientIp(clientIp), _sessionIsNew(false) {}

Client::~Client() {}

int         Client::getFd() const               { return _fd; }
ClientState Client::getState() const            { return _state; }
void        Client::setState(ClientState s)     { _state = s; }
bool        Client::isKeepAlive() const         { return _keepAlive; }
void        Client::setKeepAlive(bool v)        { _keepAlive = v; }
time_t      Client::getLastActivity() const     { return _lastActivity; }
void        Client::updateActivity()            { _lastActivity = time(NULL); }

HttpRequest  &Client::getRequest()              { return _request; }
HttpResponse &Client::getResponse()             { return _response; }

const std::string &Client::getSendBuffer() const    { return _sendBuffer; }
void               Client::setSendBuffer(const std::string &buf) { _sendBuffer = buf; }
void               Client::consumeSendBuffer(size_t n) {
    if (n >= _sendBuffer.size())
        _sendBuffer.clear();
    else
        _sendBuffer.erase(0, n);
}
bool Client::hasPendingSend() const { return !_sendBuffer.empty(); }

CgiHandler &Client::getCgi() { return _cgi; }

void Client::setCgiBodyToWrite(const std::string &body) { _cgiBodyToWrite = body; }
const std::string &Client::getCgiBodyToWrite() const    { return _cgiBodyToWrite; }
void Client::consumeCgiBody(size_t n) {
    if (n >= _cgiBodyToWrite.size())
        _cgiBodyToWrite.clear();
    else
        _cgiBodyToWrite.erase(0, n);
}

std::string &Client::getCgiOutput() { return _cgiOutput; }

const std::string &Client::getClientIp() const { return _clientIp; }
int                Client::getListenFd() const  { return _listenFd; }

void Client::setSessionInfo(const std::string &id, bool isNew) {
    _sessionId = id;
    _sessionIsNew = isNew;
}
const std::string &Client::getSessionId() const { return _sessionId; }
bool               Client::isSessionNew() const { return _sessionIsNew; }
