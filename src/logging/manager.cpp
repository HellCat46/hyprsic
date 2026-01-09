#include "manager.hpp"
#include "glib.h"
#include <chrono>
#include <cstring>
#include <ctime>
#include <iostream>

LoggingManager::LoggingManager(bool writeConsole) {
  this->writeConsole = writeConsole;
  const char *xshPath = std::getenv("XDG_STATE_HOME");

  std::string logPath;
  if (xshPath) {
    logPath = std::string(xshPath) + "/hyprsic/";
  } else {
    const char *homePath = std::getenv("HOME");
    if (homePath) {
      logPath = std::string(homePath) + "/.local/state/hyprsic/";
    }
  }

  if (logPath.size() != 0) {
    g_mkdir_with_parents(logPath.c_str(), 0755);
  }
  logPath += "hyprsic.log";

  outputStream = new std::ofstream(logPath, std::ios::app);
}

LoggingManager::~LoggingManager() {
  if (outputStream) {
    outputStream->close();
  }
}

void LoggingManager::LogInfo(const char *tag, const std::string &message) {
  *outputStream << "[INFO] [" << GetCurrentTime() << "] [" << tag << "] "
                << message << std::endl;

  if (writeConsole) {
    std::cout << "[INFO] [" << GetCurrentTime() << "] [" << tag << "] "
              << message << std::endl;
  }
}

void LoggingManager::LogWarning(const char *tag, const std::string &message) {
  *outputStream << "[WARNING] [" << GetCurrentTime() << "] [" << tag << "] "
                << message << std::endl;

  if (writeConsole) {
    std::cout << "[WARNING] [" << GetCurrentTime() << "] [" << tag << "] "
              << message << std::endl;
  }
}

void LoggingManager::LogError(const char *tag, const std::string &message) {
  *outputStream << "[ERROR] [" << GetCurrentTime() << "] [" << tag << "] "
                << message << std::endl;

  if (writeConsole) {
    std::cerr << "[ERROR] [" << GetCurrentTime() << "] [" << tag << "] "
              << message << std::endl;
  }
}

std::string LoggingManager::GetCurrentTime() {
  auto timeNow = std::chrono::system_clock::now();
  std::time_t currentTime = std::chrono::system_clock::to_time_t(timeNow);
  char *timeStr = std::ctime(&currentTime);
  timeStr[std::strlen(timeStr) - 1] = '\0'; // Remove the newline character
  return timeStr;
}
