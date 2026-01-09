#pragma once

#include "string"
#include "sys/statvfs.h"
#include "../../logging/manager.hpp"

class Disk {
private:
  std::string mountpoint;
  struct statvfs data;
  LoggingManager *logger;

public:
  int Init(std::string path, LoggingManager *logMgr);

  int GetDiskInfo(unsigned long &avail, unsigned long &tot);
};