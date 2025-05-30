#pragma once
#include "cstring"
#include "fstream"

class NetInterface {
private:
  std::string interface;
  std::ifstream rx;
  std::ifstream tx;

public:
  std::string err;
  bool up;
  NetInterface();
  int Init(std::string ifaceName);

  unsigned long GetTotRxBytes();
  unsigned long GetTotTxBytes();

  ~NetInterface();
};

class Network {
private:
  NetInterface *activeIfaces;
  int ifaceSize = 0;

public:
  double GetTotRx();

  double GetTotTx();
  int Init();
  ~Network();
};
