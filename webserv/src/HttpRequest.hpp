#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <vector>

enum RequestState {
    REQ_LINE,
    REQ_HEADERS,
    REQ_BODY,
    REQ_COMPLETE,
    REQ_ERROR
};

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    void reset();

    // Feed raw data; returns true when complete
    bool feed(const char *data, size_t len, size_t maxBodySize);

    RequestState       getState() const;
    bool               isError() const;
    int                getErrorCode() const;

    const std::string &getMethod() const;
    const std::string &getUri() const;
    const std::string &getPath() const;
    const std::string &getQueryString() const;
    const std::string &getHttpVersion() const;
    const std::string &getHeader(const std::string &name) const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::string &getBody() const;
    bool               isKeepAlive() const;
    bool               isChunked() const;
    size_t             getContentLength() const;

private:
    RequestState                        _state;
    int                                 _errorCode;
    std::string                         _buffer;
    std::string                         _method;
    std::string                         _uri;
    std::string                         _path;
    std::string                         _queryString;
    std::string                         _httpVersion;
    std::map<std::string, std::string>  _headers;
    std::string                         _body;
    size_t                              _contentLength;
    bool                                _chunked;
    bool                                _keepAlive;
    size_t                              _maxBodySize;
    // True once the last-chunk marker ("0\r\n") has been consumed and only
    // the (usually-empty) trailer section remains. Must persist across
    // parseChunked() calls: if the trailer's terminating CRLF hasn't fully
    // arrived yet, parseChunked() returns to wait for more data, and the
    // next call must resume the trailer scan rather than re-parsing from
    // scratch as if a new chunk-size line were starting.
    bool                                _inChunkTrailer;

    bool parseRequestLine(const std::string &line);
    bool parseHeader(const std::string &line);
    bool parseChunked();
    void setError(int code);
};

#endif
