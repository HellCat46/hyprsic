#pragma once

#include "../../app/context.hpp"
#include <fstream>

class BrightnessManager {
  AppContext *ctx;
  std::ifstream blFile; // Backlight file
  std::string subsystem;

  
  short currentLvl;
  bool err;

public:
  BrightnessManager(AppContext *ctx);
  
  
  void update();
  bool setLvl(short brightness);
  short getLvl() const; 
  ~BrightnessManager();
};
