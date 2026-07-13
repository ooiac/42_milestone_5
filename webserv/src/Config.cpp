#include "Config.hpp"
#include "Utils.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

Config::Config() {}
Config::~Config() {}

const std::vector<ServerConfig> &Config::getServers() const {
    return _servers;
}

std::vector<std::string> Config::tokenize(const std::string &content) {
    std::vector<std::string> tokens;
    size_t i = 0;
    while (i < content.size()) {
        // skip whitespace
        while (i < content.size() && isspace(static_cast<unsigned char>(content[i])))
            ++i;
        if (i >= content.size()) break;
        // skip comments
        if (content[i] == '#') {
            while (i < content.size() && content[i] != '\n')
                ++i;
            continue;
        }
        // special single characters
        if (content[i] == '{' || content[i] == '}' || content[i] == ';') {
            tokens.push_back(std::string(1, content[i]));
            ++i;
            continue;
        }
        // quoted string
        if (content[i] == '"' || content[i] == '\'') {
            char quote = content[i];
            ++i;
            std::string tok;
            while (i < content.size() && content[i] != quote) {
                tok += content[i];
                ++i;
            }
            if (i < content.size()) ++i;
            tokens.push_back(tok);
            continue;
        }
        // regular token
        std::string tok;
        while (i < content.size() && !isspace(static_cast<unsigned char>(content[i]))
               && content[i] != '{' && content[i] != '}'
               && content[i] != ';' && content[i] != '#') {
            tok += content[i];
            ++i;
        }
        if (!tok.empty())
            tokens.push_back(tok);
    }
    return tokens;
}

bool Config::parse(const std::string &filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "Config: cannot open file: " << filename << std::endl;
        return false;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    std::string content = oss.str();
    file.close();

    std::vector<std::string> tokens = tokenize(content);
    size_t i = 0;
    while (i < tokens.size()) {
        if (tokens[i] == "server") {
            ++i;
            if (i >= tokens.size() || tokens[i] != "{") {
                std::cerr << "Config: expected '{' after 'server'" << std::endl;
                return false;
            }
            ++i;
            if (!parseServer(tokens, i))
                return false;
        } else {
            std::cerr << "Config: unexpected token: " << tokens[i] << std::endl;
            return false;
        }
    }
    if (_servers.empty()) {
        std::cerr << "Config: no server blocks found" << std::endl;
        return false;
    }
    return true;
}

bool Config::parseServer(std::vector<std::string> &tokens, size_t &i) {
    ServerConfig srv;

    while (i < tokens.size() && tokens[i] != "}") {
        std::string key = tokens[i];
        ++i;

        if (key == "listen") {
            if (i >= tokens.size()) return false;
            std::string val = tokens[i++];
            // skip semicolon
            if (i < tokens.size() && tokens[i] == ";") ++i;
            // parse host:port or just port
            size_t colon = val.rfind(':');
            if (colon != std::string::npos) {
                srv.host = val.substr(0, colon);
                srv.port = atoi(val.substr(colon + 1).c_str());
            } else {
                srv.port = atoi(val.c_str());
            }
        } else if (key == "server_name") {
            while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}") {
                srv.serverNames.push_back(tokens[i]);
                ++i;
            }
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "client_max_body_size") {
            if (i >= tokens.size()) return false;
            srv.clientMaxBodySize = parseBodySize(tokens[i++]);
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "error_page") {
            if (i + 1 >= tokens.size()) return false;
            int code = atoi(tokens[i].c_str());
            ++i;
            std::string page = tokens[i++];
            srv.errorPages[code] = page;
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "location") {
            if (i >= tokens.size()) return false;
            LocationConfig loc;
            loc.path = tokens[i++];
            if (i >= tokens.size() || tokens[i] != "{") {
                std::cerr << "Config: expected '{' after location path" << std::endl;
                return false;
            }
            ++i;
            if (!parseLocation(tokens, i, loc))
                return false;
            srv.locations.push_back(loc);
        } else if (key == ";") {
            // stray semicolon, ignore
        } else {
            // skip unknown directives
            while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}") ++i;
            if (i < tokens.size() && tokens[i] == ";") ++i;
        }
    }
    if (i < tokens.size() && tokens[i] == "}") ++i;
    _servers.push_back(srv);
    return true;
}

bool Config::parseLocation(std::vector<std::string> &tokens, size_t &i, LocationConfig &loc) {
    while (i < tokens.size() && tokens[i] != "}") {
        std::string key = tokens[i];
        ++i;

        if (key == "root") {
            if (i >= tokens.size()) return false;
            loc.root = tokens[i++];
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "index") {
            if (i >= tokens.size()) return false;
            loc.index = tokens[i++];
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "methods" || key == "allow_methods" || key == "limit_except") {
            while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}") {
                loc.methods.push_back(tokens[i]);
                ++i;
            }
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "autoindex") {
            if (i >= tokens.size()) return false;
            loc.autoindex = (tokens[i] == "on");
            ++i;
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "upload_path") {
            if (i >= tokens.size()) return false;
            loc.uploadPath = tokens[i++];
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "client_max_body_size") {
            if (i >= tokens.size()) return false;
            loc.clientMaxBodySize = parseBodySize(tokens[i++]);
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "return") {
            if (i >= tokens.size()) return false;
            // could be: return 301 url; or return url;
            std::string first = tokens[i++];
            int code = atoi(first.c_str());
            if (code >= 100 && code < 600 && i < tokens.size() && tokens[i] != ";") {
                loc.redirectCode = code;
                loc.redirect = tokens[i++];
            } else {
                loc.redirectCode = 302;
                loc.redirect = first;
            }
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == "cgi") {
            // cgi .ext /path/to/interpreter;
            if (i + 1 >= tokens.size()) return false;
            std::string ext = tokens[i++];
            std::string interp = tokens[i++];
            loc.cgiExtensions[ext] = interp;
            if (i < tokens.size() && tokens[i] == ";") ++i;
        } else if (key == ";") {
            // stray semicolon
        } else {
            // skip unknown
            while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}") ++i;
            if (i < tokens.size() && tokens[i] == ";") ++i;
        }
    }
    if (i < tokens.size() && tokens[i] == "}") ++i;
    return true;
}

size_t Config::parseBodySize(const std::string &s) {
    if (s.empty()) return 0;
    size_t num = static_cast<size_t>(atol(s.c_str()));
    char last = s[s.size() - 1];
    if (last == 'k' || last == 'K') return num * 1024;
    if (last == 'm' || last == 'M') return num * 1024 * 1024;
    if (last == 'g' || last == 'G') return num * 1024 * 1024 * 1024;
    return num;
}
