#include "Utils.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <cstring>

std::string intToString(int n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string sizeToString(size_t n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string trim(const std::string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLower(const std::string &s) {
    std::string result = s;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(tolower(static_cast<unsigned char>(result[i])));
    return result;
}

std::string toUpper(const std::string &s) {
    std::string result = s;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = static_cast<char>(toupper(static_cast<unsigned char>(result[i])));
    return result;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::string token;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) {
            result.push_back(token);
            token.clear();
        } else {
            token += s[i];
        }
    }
    result.push_back(token);
    return result;
}

std::vector<std::string> splitWhitespace(const std::string &s) {
    std::vector<std::string> result;
    std::istringstream iss(s);
    std::string token;
    while (iss >> token)
        result.push_back(token);
    return result;
}

bool startsWith(const std::string &str, const std::string &prefix) {
    if (prefix.size() > str.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string &str, const std::string &suffix) {
    if (suffix.size() > str.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string joinPath(const std::string &a, const std::string &b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    std::string result = a;
    if (result[result.size() - 1] == '/' && b[0] == '/')
        result += b.substr(1);
    else if (result[result.size() - 1] != '/' && b[0] != '/')
        result += "/" + b;
    else
        result += b;
    return result;
}

std::string getExtension(const std::string &path) {
    size_t dot = path.rfind('.');
    size_t slash = path.rfind('/');
    if (dot == std::string::npos) return "";
    if (slash != std::string::npos && dot < slash) return "";
    return path.substr(dot);
}

std::string getMimeType(const std::string &path) {
    std::string ext = toLower(getExtension(path));
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".xml")  return "application/xml";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".pdf")  return "application/pdf";
    if (ext == ".zip")  return "application/zip";
    if (ext == ".tar")  return "application/x-tar";
    if (ext == ".gz")   return "application/gzip";
    if (ext == ".mp4")  return "video/mp4";
    if (ext == ".mp3")  return "audio/mpeg";
    if (ext == ".wav")  return "audio/wav";
    return "application/octet-stream";
}

std::string urlDecode(const std::string &s) {
    std::string result;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            std::string hex = s.substr(i + 1, 2);
            char ch = static_cast<char>(strtol(hex.c_str(), NULL, 16));
            result += ch;
            i += 2;
        } else if (s[i] == '+') {
            result += ' ';
        } else {
            result += s[i];
        }
    }
    return result;
}

std::string getCurrentDate() {
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buf);
}

bool fileExists(const std::string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool isDirectory(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

bool isRegularFile(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

long getFileSize(const std::string &path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return -1;
    return static_cast<long>(st.st_size);
}

std::string readFile(const std::string &path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

std::string generateDirectoryListing(const std::string &dirPath, const std::string &uriPath) {
    DIR *dir = opendir(dirPath.c_str());
    if (!dir) return "";

    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<title>Index of " << uriPath << "</title>\n";
    html << "<style>\n";
    html << "body { font-family: monospace; background: #1a1a2e; color: #e0e0e0; padding: 20px; }\n";
    html << "h1 { color: #7b8cde; border-bottom: 1px solid #444; padding-bottom: 10px; }\n";
    html << "table { width: 100%; border-collapse: collapse; }\n";
    html << "th { text-align: left; padding: 8px; color: #a0aec0; border-bottom: 1px solid #333; }\n";
    html << "td { padding: 6px 8px; border-bottom: 1px solid #222; }\n";
    html << "a { color: #7b8cde; text-decoration: none; }\n";
    html << "a:hover { text-decoration: underline; color: #a3b0ff; }\n";
    html << ".dir { color: #f6ad55; }\n";
    html << "</style>\n</head>\n<body>\n";
    html << "<h1>Index of " << uriPath << "</h1>\n";
    html << "<table>\n<tr><th>Name</th><th>Size</th><th>Type</th></tr>\n";

    // Parent directory link
    if (uriPath != "/") {
        html << "<tr><td><a href=\"../\">..</a></td><td>-</td><td>Directory</td></tr>\n";
    }

    struct dirent *entry;
    std::vector<std::string> entries;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") continue;
        entries.push_back(name);
    }
    closedir(dir);

    std::sort(entries.begin(), entries.end());

    for (size_t i = 0; i < entries.size(); ++i) {
        const std::string &name = entries[i];
        std::string fullPath = joinPath(dirPath, name);
        std::string displayName = name;
        std::string size = "-";
        std::string type = "File";

        if (isDirectory(fullPath)) {
            displayName += "/";
            type = "Directory";
            html << "<tr><td class=\"dir\"><a href=\"" << name << "/\">" << displayName << "</a></td>";
        } else {
            long sz = getFileSize(fullPath);
            if (sz >= 0) {
                if (sz < 1024)
                    size = sizeToString(static_cast<size_t>(sz)) + " B";
                else if (sz < 1024 * 1024)
                    size = sizeToString(static_cast<size_t>(sz / 1024)) + " KB";
                else
                    size = sizeToString(static_cast<size_t>(sz / (1024 * 1024))) + " MB";
            }
            html << "<tr><td><a href=\"" << name << "\">" << displayName << "</a></td>";
        }
        html << "<td>" << size << "</td><td>" << type << "</td></tr>\n";
    }

    html << "</table>\n";
    html << "<hr><p style=\"color:#666\">webserv/1.0</p>\n";
    html << "</body>\n</html>\n";
    return html.str();
}
