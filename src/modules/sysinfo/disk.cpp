#include "disk.hpp"
#include "sys/statvfs.h"

#define TAG "Disk"

int Disk::Init(std::string path, LoggingManager *logMgr) {
  logger = logMgr;
  
  if (statvfs(path.c_str(), &data) < 0) {
    logger->LogError(TAG, "Failed to Get Info about the Path.");
    return 1;
  }

  mountpoint = path;
  return 0;
}

int Disk::GetDiskInfo(unsigned long& avail, unsigned long& tot) {
  if (statvfs(mountpoint.c_str(), &data) < 0) {
    logger->LogError(TAG, "Failed to Get Info about the Path.");
    return 1;
  }

  avail = data.f_bavail * data.f_bsize;
  tot = data.f_blocks * data.f_bsize;
  return 0;
}