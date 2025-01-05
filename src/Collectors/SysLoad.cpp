#include "iostream"
#include "fstream"
#include "cstring"
#include "cmath"


class SysLoad {
  private:
  std::ifstream loadAvg;

  public:
    int Init() {
      loadAvg.open("/proc/loadavg", std::ios::in);
      if(!loadAvg.is_open()){
        std::cerr<<"Failed to Open Load Avg File";
        return 0;
      }
      return 0;
    }

    float GetLoad(int dur){
      float load = 0;

      int jump;
      if(dur == 15) jump = 2;
      else if(dur == 5) jump = 1;
      else if(dur == 1) jump = 0;
      else {
        std::cerr<<"Invalid Time Duration for System Load."<<std::endl;
        return -1;
      }

      char ch;
      bool point = false;
      short multi = 1;
      while(loadAvg.get(ch)) {
        if(ch == ' '){
          if(jump == 0) break;
          else jump--;
        }

        if(jump == 0){
          if(ch>=48 && ch<=57){
            if(!point) load = (load*10) + (ch-48);
            else load += (ch-48)/ std::pow(10, multi++);
          }else if(ch == '.') point = ch == '.';
        }
      }
      loadAvg.clear();
      loadAvg.seekg(0);
      return load;
    }

    ~SysLoad(){
      loadAvg.close();
    }
};