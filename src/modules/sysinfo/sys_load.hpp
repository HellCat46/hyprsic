#pragma once
#include "../../logging/manager.hpp"
#include "cmath"
#include "cstring"
#include "fstream"

class SysLoad {
private:
  std::ifstream loadAvg;
  LoggingManager *logger;

public:
  int Init(LoggingManager *logMgr);
  float GetLoad(int dur);
  ~SysLoad();
};
