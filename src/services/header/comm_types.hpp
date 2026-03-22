#pragma once

#include <cstdint>
#include <string>

enum class Priority : uint8_t {
    LOW  = 0,
    NORMAL = 1,
    HIGH = 2,
    IMMEDIATE = 255
};

struct ResponseMessage {
    bool success;
    std::string errMsg;
    uint64_t correlationId;
};

struct ResponseWrapper {
    ResponseMessage msg;
    Priority priority;
};

struct ResponseWrapperCmp {
    bool operator()(const ResponseWrapper& a, const ResponseWrapper& b) const {
        return a.priority < b.priority;
    }  
};