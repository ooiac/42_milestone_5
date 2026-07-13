#include "SessionManager.hpp"
#include "Utils.hpp"
#include <cstdlib>
#include <unistd.h>

// Reads "name=value" pairs out of a raw Cookie header ("a=1; b=2") and
// returns the value for the requested name, or "" if absent.
static std::string extractCookie(const std::string &cookieHeader, const std::string &name) {
    std::vector<std::string> parts = split(cookieHeader, ';');
    for (size_t i = 0; i < parts.size(); ++i) {
        std::string part = trim(parts[i]);
        size_t eq = part.find('=');
        if (eq == std::string::npos) continue;
        if (trim(part.substr(0, eq)) == name)
            return trim(part.substr(eq + 1));
    }
    return "";
}

SessionManager::SessionManager() {
    srand(static_cast<unsigned int>(time(NULL)) ^ static_cast<unsigned int>(getpid()));
}

SessionManager::~SessionManager() {}

std::string SessionManager::generateId() {
    static const char hex[] = "0123456789abcdef";
    std::string id;
    id.reserve(SESSION_ID_LEN);
    for (int i = 0; i < SESSION_ID_LEN; ++i)
        id += hex[rand() % 16];
    return id;
}

SessionData &SessionManager::getOrCreate(const std::string &cookieHeader, bool &isNew) {
    std::string sid = extractCookie(cookieHeader, SESSION_COOKIE_NAME);
    if (!sid.empty()) {
        std::map<std::string, SessionData>::iterator it = _sessions.find(sid);
        if (it != _sessions.end()) {
            it->second.lastAccess = time(NULL);
            isNew = false;
            return it->second;
        }
    }

    std::string newId;
    do {
        newId = generateId();
    } while (_sessions.find(newId) != _sessions.end());

    SessionData sess;
    sess.id         = newId;
    sess.createdAt  = time(NULL);
    sess.lastAccess = sess.createdAt;
    sess.visits     = 0;
    _sessions[newId] = sess;
    isNew = true;
    return _sessions[newId];
}

SessionData *SessionManager::find(const std::string &id) {
    std::map<std::string, SessionData>::iterator it = _sessions.find(id);
    if (it == _sessions.end()) return NULL;
    return &it->second;
}

void SessionManager::cleanupExpired() {
    time_t now = time(NULL);
    std::vector<std::string> expired;
    for (std::map<std::string, SessionData>::iterator it = _sessions.begin();
         it != _sessions.end(); ++it) {
        if (now - it->second.lastAccess > SESSION_TIMEOUT_SEC)
            expired.push_back(it->first);
    }
    for (size_t i = 0; i < expired.size(); ++i)
        _sessions.erase(expired[i]);
}
