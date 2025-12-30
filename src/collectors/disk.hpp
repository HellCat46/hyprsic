#pragma once

#include "string"
#include "sys/statvfs.h"

class Disk {
private:
  std::string mountpoint;
  struct statvfs data;

public:
  int Init(std::string path);

  int GetDiskInfo(unsigned long &avail, unsigned long &tot);
};
