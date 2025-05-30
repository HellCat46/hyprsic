#pragma once
#include "cstring"
#include "fstream"

class Memory {
private:
  std::ifstream memInfo;
  long parseInfo(std::string type);

public:
  int Init();
  long GetUsedRAM();
  long GetTotRAM();
  long GetUsedSwap();
  long GetTotSwap();
  ~Memory();
};
