#pragma once
#include "../../../logging/manager.hpp"
#include "cstring"
#include "fstream"
#include <memory>
#include <string>
#include <vector>

class NetInterface {
private:
  std::ifstream rx;
  std::ifstream tx;
  LoggingManager *logger;

public:
  std::string err;
  std::string interface;
  bool up;
  
  NetInterface(std::string ifaceName, LoggingManager *logMgr);
  unsigned long GetTotRxBytes();
  unsigned long GetTotTxBytes();
  ~NetInterface();
};

class Network {
private:
  std::vector<std::unique_ptr<NetInterface>> activeIfaces;
  int ifaceSize = 0;
  LoggingManager *logger;

public:
  Network(LoggingManager *logMgr);
  double GetTotRx();
  double GetTotTx();
  std::string GetIfaces();
};
