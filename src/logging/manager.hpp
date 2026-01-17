#pragma once

#include <fstream>
#include <string>
class LoggingManager {
    std::ofstream outputStream;
    bool writeConsole;
    
    std::string GetCurrentTime();
    
  public:
    LoggingManager(bool writeConsole = false);
    ~LoggingManager();
    void LogInfo(const char* tag, const std::string& message);
    void LogWarning(const char* tag, const std::string& message);
    void LogError(const char* tag, const std::string& message);
    void LogDebug(const char* tag, const std::string& message);
};