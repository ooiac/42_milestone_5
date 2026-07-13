#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string                        path;
    std::string                        root;
    std::string                        index;
    std::vector<std::string>           methods;
    bool                               autoindex;
    std::string                        uploadPath;
    std::string                        redirect;      // return redirect URL
    int                                redirectCode;  // 301, 302, etc.
    std::map<std::string, std::string> cgiExtensions; // ext -> interpreter
    size_t                             clientMaxBodySize; // 0 = inherit from server

    LocationConfig()
        : autoindex(false), redirectCode(0), clientMaxBodySize(0) {}
};

struct ServerConfig {
    std::string                    host;
    int                            port;
    std::vector<std::string>       serverNames;
    size_t                         clientMaxBodySize;
    std::map<int, std::string>     errorPages;
    std::vector<LocationConfig>    locations;

    ServerConfig()
        : host("0.0.0.0"), port(80), clientMaxBodySize(1024 * 1024) {}
};

class Config {
public:
    Config();
    ~Config();

    bool parse(const std::string &filename);
    const std::vector<ServerConfig> &getServers() const;

private:
    std::vector<ServerConfig> _servers;

    bool parseServer(std::vector<std::string> &tokens, size_t &i);
    bool parseLocation(std::vector<std::string> &tokens, size_t &i, LocationConfig &loc);
    size_t parseBodySize(const std::string &s);
    std::vector<std::string> tokenize(const std::string &content);
};

#endif
