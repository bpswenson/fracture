#pragma once

#include <queue>
#include <stack>
#include <string>
#include <functional>

struct Event {
    double timestamp;
    void (*handler)(void*);      // Forward event handler
    void (*reverseHandler)(void*); // Reverse event handler
    void *data;                  // Event-specific data
};

// Comparator for Event based on timestamp
struct EventCompare {
    bool operator()(const Event& lhs, const Event& rhs) const {
        return lhs.timestamp > rhs.timestamp;
    }
};
/*
class Simulator {
    std::priority_queue<Event, std::vector<Event>, EventCompare> eventQueue;
    std::stack<Event> executedEvents;

public:
    void scheduleEvent(double timestamp, void (*handler)(void*), const std::string &reverseFunctionName, void *data);

    void run(double endTime);
    void rollback(double rollbackTime);

private:
    void* loadReverseHandler(const char* handlerName, const char* libPath);    
};
*/
