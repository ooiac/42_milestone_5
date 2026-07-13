#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include <string>
#include <map>
#include <ctime>

#define SESSION_COOKIE_NAME "session_id"
#define SESSION_TIMEOUT_SEC 600
#define SESSION_ID_LEN      32

struct SessionData {
    std::string id;
    time_t      createdAt;
    time_t      lastAccess;
    int         visits;

    SessionData() : createdAt(0), lastAccess(0), visits(0) {}
};

// In-memory session store, keyed by the id handed out in the
// "session_id" cookie. Sessions idle for longer than SESSION_TIMEOUT_SEC
// are dropped the next time cleanupExpired() runs.
class SessionManager {
public:
    SessionManager();
    ~SessionManager();

    // Looks up the session named in the request's raw Cookie header.
    // If none is found (missing cookie, unknown id, or expired), a fresh
    // session is created. isNew reports which case happened, so the
    // caller knows whether a Set-Cookie needs to go out with the response.
    SessionData &getOrCreate(const std::string &cookieHeader, bool &isNew);

    SessionData *find(const std::string &id);

    void cleanupExpired();

private:
    std::map<std::string, SessionData> _sessions;

    std::string generateId();
};

#endif
