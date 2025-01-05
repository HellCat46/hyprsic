#include "iostream"
#include "math.h"
#include "chrono"
#include "format"
#include "Collectors/Network.cpp"
#include "Collectors/SysLoad.cpp"

struct Stats {
  double rx, tx;
  std::chrono::time_point<std::chrono::steady_clock> time;
};

class Display {
  Stats stat;
  Network net;
  SysLoad load;
  public:
   Display() {
     // Initiating The Network and It's Stats
     net.Init();
     stat.rx = net.GetTotRx();
     stat.tx = net.GetTotTx();
     stat.time = std::chrono::steady_clock::now();

     load.Init();
   }
  std::string DisplayBar(){
    std::string str;

    auto curTime = std::chrono::steady_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(curTime - stat.time).count();


    double trx = net.GetTotRx(), ttx = net.GetTotTx();
    str += DisplayBytes(trx) + " (" + DisplayBytes((trx-stat.rx)/sec) + ")\t"+ DisplayBytes(ttx) + " ("+DisplayBytes((ttx-stat.tx)/sec)+ ")\t";
    str += std::to_string(load.GetLoad(1)) +" "+std::to_string(load.GetLoad(5)) +" "+std::to_string(load.GetLoad(15)) +" "+"\n";

    stat.rx = trx;
    stat.tx = ttx;
    stat.time = curTime;
    return str;
  }
  std::string DisplayBytes(double bytes){
    std::string str;

    if(bytes>pow(1024,3)){ // GigaBytes
      str = std::to_string(bytes/pow(1024,3));
      str = str.substr(0, str.find('.')+3);
      str += " GiB";
    }else if(bytes>pow(1024,2)){ // MegaBytes
      str = std::to_string(bytes/pow(1024,2));
      str = str.substr(0, str.find('.')+3);
      str +=  " MiB";
    }else if(bytes>1024){ // KiloBytes
      str = std::to_string(bytes/1024);
      str = str.substr(0, str.find('.')+3);
      str +=  " KiB";
    }else if(bytes>0) { // Bytes
      str = std::to_string(bytes);
      str = str.substr(0, str.find('.'));
      str +=  " B";
    }else {
      str += "0 B";
    }
    return str;
  }
};
