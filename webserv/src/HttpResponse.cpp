#include "HttpResponse.hpp"
#include "Utils.hpp"
#include <sstream>

HttpResponse::HttpResponse() : _statusCode(200), _statusMsg("OK") {}
HttpResponse::~HttpResponse() {}

void HttpResponse::setStatus(int code, const std::string &message) {
    _statusCode = code;
    if (message.empty())
        _statusMsg = statusMessage(code);
    else
        _statusMsg = message;
}

void HttpResponse::setHeader(const std::string &name, const std::string &value) {
    _headers[name] = value;
}

void HttpResponse::setBody(const std::string &body) {
    _body = body;
}

void HttpResponse::setBody(const std::string &body, const std::string &contentType) {
    _body = body;
    _headers["Content-Type"] = contentType;
}

int HttpResponse::getStatusCode() const {
    return _statusCode;
}

std::string HttpResponse::build() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << _statusCode << " " << _statusMsg << "\r\n";
    oss << "Date: " << getCurrentDate() << "\r\n";
    oss << "Server: webserv/1.0\r\n";

    // Write all headers
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        oss << it->first << ": " << it->second << "\r\n";
    }

    // Content-Length if not already set
    if (_headers.find("Content-Length") == _headers.end()) {
        oss << "Content-Length: " << _body.size() << "\r\n";
    }

    oss << "\r\n";
    oss << _body;
    return oss.str();
}

std::string HttpResponse::buildError(int code, const std::string &customPage) {
    setStatus(code);
    std::string body;
    if (!customPage.empty())
        body = customPage;
    else
        body = buildErrorPage(code);
    setBody(body, "text/html");
    return build();
}

std::string HttpResponse::statusMessage(int code) {
    switch (code) {
        case 100: return "Continue";
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 206: return "Partial Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 422: return "Unprocessable Entity";
        case 429: return "Too Many Requests";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

std::string HttpResponse::buildErrorPage(int code) {
    std::string msg = statusMessage(code);
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<title>" << code << " " << msg << "</title>\n";
    html << "<style>\n";
    html << "body{font-family:sans-serif;background:#1a1a2e;color:#e0e0e0;display:flex;";
    html << "flex-direction:column;align-items:center;justify-content:center;height:100vh;margin:0;}\n";
    html << "h1{font-size:6rem;color:#7b8cde;margin:0;}\n";
    html << "p{font-size:1.5rem;color:#a0aec0;}\n";
    html << "a{color:#7b8cde;}\n";
    html << "</style>\n</head>\n<body>\n";
    html << "<h1>" << code << "</h1>\n";
    html << "<p>" << msg << "</p>\n";
    html << "<a href=\"/\">Go Home</a>\n";
    html << "</body>\n</html>\n";
    return html.str();
}
