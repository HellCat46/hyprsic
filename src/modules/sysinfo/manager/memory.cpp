#include "memory.hpp"
#include "cstring"
#include "fstream"

#define TAG "Memory"

int Memory::Init(LoggingManager *logMgr) {
  logger = logMgr;

  memInfo.open("/proc/meminfo", std::ios::in);
  if (!memInfo.is_open()) {
    logger->LogError(TAG, "Failed to Open Memory Info");
    return 1;
  }
  return 0;
}

long Memory::GetUsedRAM() {
  return parseInfo("MemTotal") - parseInfo("MemAvailable");
}
long Memory::GetTotRAM() { return parseInfo("MemTotal"); }
long Memory::GetUsedSwap() {
  return parseInfo("SwapTotal") - parseInfo("SwapFree");
}
long Memory::GetTotSwap() { return parseInfo("SwapTotal"); }

Memory::~Memory() { memInfo.close(); }

long Memory::parseInfo(std::string type) {
  long amt = 0;
  char line[100];

  while (memInfo.getline(line, 100)) {
    if (std::strncmp(line, type.c_str(), type.length()) == 0) {
      int idx = 0;
      while (line[idx] != '\0') {
        if (line[idx] >= 48 && line[idx] <= 57) {
          amt = (amt * 10) + (line[idx] - 48);
        }
        idx++;
      }
      break;
    }
  }
  memInfo.clear();
  memInfo.seekg(0);

  return amt;
}
