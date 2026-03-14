#pragma once

#include "services/header/context.hpp"
#include <fstream>

class BrightnessManager {
  AppContext *ctx;
  std::ifstream blFile; // Backlight file
  std::string subsystem;

  
  short currentLvl;
  bool err;
  
  
  bool setLvl(short brightness);
  
  void update();

public:
  BrightnessManager(AppContext *ctx);
  
  short getLvl() const; 
  ~BrightnessManager();
};
