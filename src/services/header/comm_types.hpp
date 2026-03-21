#pragma once

#include <string>

struct Request {
  std::byte priority;

  bool operator()(Request l, Request r) const {
    return l.priority > r.priority;
  }
};

struct Response {
  std::byte priority;

  bool operator()(Response l, Response r) const {
    return l.priority > r.priority;
  }
};


struct ResponseMessage {
    bool success;
    std::string errMsg;
    long int correlationId;
};