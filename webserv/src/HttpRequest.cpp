#include "HttpRequest.hpp"
#include "Utils.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>

HttpRequest::HttpRequest()
    : _state(REQ_LINE), _errorCode(0), _contentLength(0),
      _chunked(false), _keepAlive(false), _maxBodySize(1024 * 1024),
      _inChunkTrailer(false) {}

HttpRequest::~HttpRequest() {}

void HttpRequest::reset() {
    _state = REQ_LINE;
    _errorCode = 0;
    // _buffer is deliberately left untouched: a pipelined next request may
    // have arrived concatenated with the one just completed (both read in
    // the same recv()), and parsing already left exactly those leftover
    // bytes in place. Discarding them here would silently drop that request.
    _method.clear();
    _uri.clear();
    _path.clear();
    _queryString.clear();
    _httpVersion.clear();
    _headers.clear();
    _body.clear();
    _contentLength = 0;
    _chunked = false;
    _keepAlive = false;
    _inChunkTrailer = false;
}

void HttpRequest::setError(int code) {
    _state = REQ_ERROR;
    _errorCode = code;
}

RequestState HttpRequest::getState() const       { return _state; }
bool         HttpRequest::isError() const        { return _state == REQ_ERROR; }
int          HttpRequest::getErrorCode() const   { return _errorCode; }
const std::string &HttpRequest::getMethod() const       { return _method; }
const std::string &HttpRequest::getUri() const          { return _uri; }
const std::string &HttpRequest::getPath() const         { return _path; }
const std::string &HttpRequest::getQueryString() const  { return _queryString; }
const std::string &HttpRequest::getHttpVersion() const  { return _httpVersion; }
const std::string &HttpRequest::getBody() const         { return _body; }
bool HttpRequest::isKeepAlive() const                   { return _keepAlive; }
bool HttpRequest::isChunked() const                     { return _chunked; }
size_t HttpRequest::getContentLength() const            { return _contentLength; }

const std::map<std::string, std::string> &HttpRequest::getHeaders() const {
    return _headers;
}

const std::string &HttpRequest::getHeader(const std::string &name) const {
    static std::string empty;
    std::map<std::string, std::string>::const_iterator it = _headers.find(toLower(name));
    if (it == _headers.end()) return empty;
    return it->second;
}

bool HttpRequest::feed(const char *data, size_t len, size_t maxBodySize) {
    _maxBodySize = maxBodySize;
    _buffer.append(data, len);

    while (_state != REQ_COMPLETE && _state != REQ_ERROR) {
        if (_state == REQ_LINE) {
            size_t pos = _buffer.find("\r\n");
            if (pos == std::string::npos) {
                pos = _buffer.find("\n");
                if (pos == std::string::npos) break;
            }
            std::string line = _buffer.substr(0, pos);
            _buffer.erase(0, pos + ((_buffer[pos] == '\r') ? 2 : 1));
            if (!parseRequestLine(line)) return false;
        } else if (_state == REQ_HEADERS) {
            size_t pos = _buffer.find("\r\n");
            size_t lineEnd = 2;
            if (pos == std::string::npos) {
                pos = _buffer.find("\n");
                lineEnd = 1;
                if (pos == std::string::npos) break;
            }
            std::string line = _buffer.substr(0, pos);
            _buffer.erase(0, pos + lineEnd);

            if (line.empty()) {
                // End of headers
                std::string connHdr = getHeader("connection");
                if (_httpVersion == "HTTP/1.1") {
                    _keepAlive = (connHdr.empty() || toLower(connHdr) != "close");
                } else {
                    _keepAlive = (!connHdr.empty() && toLower(connHdr) == "keep-alive");
                }

                std::string clStr = getHeader("content-length");
                if (!clStr.empty()) {
                    _contentLength = static_cast<size_t>(atol(clStr.c_str()));
                    if (_contentLength > _maxBodySize) {
                        setError(413);
                        return false;
                    }
                }

                std::string teStr = getHeader("transfer-encoding");
                if (!teStr.empty() && toLower(teStr) == "chunked") {
                    _chunked = true;
                }

                if (_method == "GET" || _method == "HEAD" || _method == "DELETE") {
                    _state = REQ_COMPLETE;
                } else if (_chunked) {
                    _state = REQ_BODY;
                } else if (_contentLength > 0) {
                    _state = REQ_BODY;
                } else {
                    _state = REQ_COMPLETE;
                }
            } else {
                if (!parseHeader(line)) return false;
            }
        } else if (_state == REQ_BODY) {
            if (_chunked) {
                if (!parseChunked()) break;
            } else {
                if (_buffer.size() >= _contentLength) {
                    _body = _buffer.substr(0, _contentLength);
                    _buffer.erase(0, _contentLength);
                    _state = REQ_COMPLETE;
                } else {
                    break;
                }
            }
        } else {
            break;
        }
    }
    return true;
}

bool HttpRequest::parseRequestLine(const std::string &line) {
    std::vector<std::string> parts = splitWhitespace(line);
    if (parts.size() != 3) {
        setError(400);
        return false;
    }
    _method = parts[0];
    _uri    = parts[1];
    _httpVersion = parts[2];

    if (_httpVersion != "HTTP/1.0" && _httpVersion != "HTTP/1.1") {
        setError(400);
        return false;
    }

    // Validate method
    if (_method != "GET" && _method != "POST" && _method != "DELETE"
        && _method != "HEAD" && _method != "PUT" && _method != "OPTIONS"
        && _method != "PATCH") {
        setError(501);
        return false;
    }

    // Parse URI: path?query
    size_t q = _uri.find('?');
    if (q != std::string::npos) {
        _path        = urlDecode(_uri.substr(0, q));
        _queryString = _uri.substr(q + 1);
    } else {
        _path = urlDecode(_uri);
    }

    _state = REQ_HEADERS;
    return true;
}

bool HttpRequest::parseHeader(const std::string &line) {
    size_t colon = line.find(':');
    if (colon == std::string::npos) {
        setError(400);
        return false;
    }
    std::string name  = toLower(trim(line.substr(0, colon)));
    std::string value = trim(line.substr(colon + 1));
    _headers[name] = value;
    return true;
}

bool HttpRequest::parseChunked() {
    while (true) {
        // Last chunk seen on a previous call: what follows is the (usually
        // empty) trailer section — zero or more "header: value" lines, each
        // ending in its own CRLF, followed by one more CRLF that terminates
        // the whole chunked body. With no trailers that's just a single
        // blank line, so trailers are consumed one line at a time rather
        // than by searching for a double CRLF. If the terminator hasn't
        // fully arrived yet we must return and pick up right back here on
        // the next call — re-entering the chunk-size parser below would
        // misread a lone leftover CRLF as a new (phantom) zero-size chunk
        // and hang waiting for one more terminator that will never come.
        if (_inChunkTrailer) {
            while (true) {
                size_t tPos = _buffer.find("\r\n");
                size_t tLineEnd = 2;
                if (tPos == std::string::npos) {
                    tPos = _buffer.find("\n");
                    tLineEnd = 1;
                    if (tPos == std::string::npos)
                        return false; // trailer terminator not fully received yet
                }
                bool blankLine = (tPos == 0);
                _buffer.erase(0, tPos + tLineEnd);
                if (blankLine) {
                    _inChunkTrailer = false;
                    _state = REQ_COMPLETE;
                    return true;
                }
            }
        }

        // Read chunk size line
        size_t pos = _buffer.find("\r\n");
        size_t lineEnd = 2;
        if (pos == std::string::npos) {
            pos = _buffer.find("\n");
            lineEnd = 1;
            if (pos == std::string::npos) return false;
        }
        std::string sizeLine = trim(_buffer.substr(0, pos));
        // Strip chunk extensions (;...)
        size_t semi = sizeLine.find(';');
        if (semi != std::string::npos) sizeLine = sizeLine.substr(0, semi);

        size_t chunkSize = static_cast<size_t>(strtol(sizeLine.c_str(), NULL, 16));

        if (chunkSize == 0) {
            _buffer.erase(0, pos + lineEnd);
            _inChunkTrailer = true;
            continue;
        }

        // Check if we have the full chunk data + \r\n
        if (_buffer.size() < pos + lineEnd + chunkSize + lineEnd)
            return false;

        if (_body.size() + chunkSize > _maxBodySize) {
            setError(413);
            return false;
        }

        _body.append(_buffer, pos + lineEnd, chunkSize);
        _buffer.erase(0, pos + lineEnd + chunkSize + lineEnd);
    }
}
