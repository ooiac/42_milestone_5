#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <sys/types.h>

std::string intToString(int n);
std::string sizeToString(size_t n);
std::string trim(const std::string &s);
std::string toLower(const std::string &s);
std::string toUpper(const std::string &s);
std::vector<std::string> split(const std::string &s, char delim);
std::vector<std::string> splitWhitespace(const std::string &s);
bool startsWith(const std::string &str, const std::string &prefix);
bool endsWith(const std::string &str, const std::string &suffix);
std::string joinPath(const std::string &a, const std::string &b);
std::string getExtension(const std::string &path);
std::string getMimeType(const std::string &path);
std::string urlDecode(const std::string &s);
std::string getCurrentDate();
bool fileExists(const std::string &path);
bool isDirectory(const std::string &path);
bool isRegularFile(const std::string &path);
long getFileSize(const std::string &path);
std::string readFile(const std::string &path);
std::string generateDirectoryListing(const std::string &dirPath, const std::string &uriPath);

#endif
