#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    void setStatus(int code, const std::string &message = "");
    void setHeader(const std::string &name, const std::string &value);
    void setBody(const std::string &body);
    void setBody(const std::string &body, const std::string &contentType);

    int         getStatusCode() const;
    std::string build() const;
    std::string buildError(int code, const std::string &customPage = "");

    static std::string statusMessage(int code);
    static std::string buildErrorPage(int code);

private:
    int                                _statusCode;
    std::string                        _statusMsg;
    std::map<std::string, std::string> _headers;
    std::string                        _body;
};

#endif
