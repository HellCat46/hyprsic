#include "network.hpp"
#include "cstring"
#include "dirent.h"
#include "fstream"
#include "iostream"
#include "vector"

NetInterface::NetInterface() { err = "Not Initialized"; }

int NetInterface::Init(std::string ifaceName) {
  std::cout << "[Init Info] Initiating Interface " << ifaceName << " Object."
            << std::endl;

  this->interface = ifaceName;
  std::string path = "/sys/class/net/" + interface;

  std::ifstream ostate(path + "/operstate", std::ios::in);
  if (!ostate.is_open()) {
    err = "Unable to check status of Interface " + interface + ".";
    return 1;
  }

  char state[5];
  ostate >> state;
  up = std::strncmp(state, "down", 4) != 0;
  ostate.close();

  rx.open(path + "/statistics/rx_bytes", std::ios::in);
  tx.open(path + "/statistics/tx_bytes", std::ios::in);
  if (!rx.is_open() || !tx.is_open()) {
    err = "Unable to Open Stat Files.";
    return 1;
  }

  err = "";
  return 0;
}

unsigned long NetInterface::GetTotRxBytes() {
  char ch;
  unsigned long bytes = 0;
  while (rx.get(ch)) {
    if (ch < 48 || ch > 57) {
      continue;
    }

    bytes = (bytes * 10) + (ch - 48);
  }
  rx.clear();
  rx.seekg(0);
  return bytes;
}

unsigned long NetInterface::GetTotTxBytes() {
  char ch;
  unsigned long bytes = 0;
  while (tx.get(ch)) {
    if (ch < 48 || ch > 57) {
      continue;
    }

    bytes = (bytes * 10) + (ch - 48);
  }
  tx.clear();
  tx.seekg(0);
  return bytes;
}

NetInterface::~NetInterface() {
  rx.close();
  tx.close();
}

double Network::GetTotRx() {
  double bytes = 0;

  for (int idx = 0; idx < ifaceSize; idx++) {
    if (activeIfaces[idx].up && activeIfaces[idx].err.length() == 0)
      bytes += activeIfaces[idx].GetTotRxBytes();
  }

  return bytes;
}

double Network::GetTotTx() {
  double bytes = 0;

  for (int idx = 0; idx < ifaceSize; idx++) {
    if (activeIfaces[idx].up && activeIfaces[idx].err.length() == 0)
      bytes += activeIfaces[idx].GetTotTxBytes();
  }

  return bytes;
}

int Network::Init() {
  DIR *dir;
  dir = opendir("/sys/class/net/");
  if (dir == nullptr) {
    std::cerr << "[Init Error] Failed to Open /sys/class/net/ Directory"
              << std::endl;
    return 1;
  }

  const std::string baseIfaces[] = {"eno", "enp",  "ens", "enx",
                                    "eth", "wlan", "wlp"};
  std::vector<std::string> availIfaces;

  for (dirent *ent = readdir(dir); ent != nullptr; ent = readdir(dir)) {
    if (ent->d_name[0] != '.') {
      for (auto baseIface : baseIfaces) {
        if (std::strncmp(ent->d_name, baseIface.c_str(), baseIface.length()) ==
            0) {
          availIfaces.push_back(ent->d_name);
        }
        // std::cout<<ent->d_name<<"\t"<<(int)ent->d_type<<std::endl;
      }
    }
  }
  closedir(dir);

  activeIfaces = new NetInterface[availIfaces.size()];
  ifaceSize = availIfaces.size();
  int idx = 0;
  for (auto ifaceName : availIfaces) {
    activeIfaces[idx++].Init(ifaceName);
  }
  return 0;
}

Network::~Network() { delete[] activeIfaces; }
