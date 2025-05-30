#pragma once
#include "cmath"
#include "cstring"
#include "fstream"

class SysLoad {
private:
  std::ifstream loadAvg;

public:
  int Init();
  float GetLoad(int dur);
  ~SysLoad();
};
