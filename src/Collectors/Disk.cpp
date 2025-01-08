#include "iostream"
#include "fstream"
#include "sys/statvfs.h"


class Disk {
  private:
    std::string mountpoint;
    struct statvfs data;

  public:
    int Init(std::string path){
      if(statvfs(path.c_str(), &data) < 0){
        std::cerr<<"[Init Error] Failed to Get Info about the Path."<<std::endl;
        return 1;
      }

      mountpoint = path;
      return 0;
    }

    int GetDiskInfo(unsigned long* avail, unsigned long* tot){
      if(statvfs(mountpoint.c_str(), &data) < 0){
        std::cerr<<"[Disk Info] Failed to Get Info about the Path."<<std::endl;
        return 1;
      }


      *avail = data.f_bavail * data.f_bsize;
      *tot = data.f_blocks * data.f_bsize;
      return 0;
    }

};