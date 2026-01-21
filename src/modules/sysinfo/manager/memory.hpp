#pragma once
#include "cstring"
#include "fstream"
#include "../../../logging/manager.hpp"

class Memory {
private:
  std::ifstream memInfo;
  LoggingManager *logger;
  long parseInfo(std::string type);

public:
  Memory(LoggingManager *logMgr);
  long GetUsedRAM();
  long GetTotRAM();
  long GetUsedSwap();
  long GetTotSwap();
  ~Memory();
};