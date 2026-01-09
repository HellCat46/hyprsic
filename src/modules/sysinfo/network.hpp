#pragma once
#include "cstring"
#include "fstream"
#include "../../logging/manager.hpp"

class NetInterface {
private:
  std::string interface;
  std::ifstream rx;
  std::ifstream tx;
  LoggingManager *logger;

public:
  std::string err;
  bool up;
  NetInterface();
  int Init(std::string ifaceName, LoggingManager *logMgr);

  unsigned long GetTotRxBytes();
  unsigned long GetTotTxBytes();

  ~NetInterface();
};

class Network {
private:
  NetInterface *activeIfaces;
  int ifaceSize = 0;
  LoggingManager *logger;

public:
  double GetTotRx();

  double GetTotTx();
  int Init(LoggingManager *logMgr);
  ~Network();
};