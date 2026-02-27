#pragma once

#include "../../app/context.hpp"
#include <fstream>

class BrightnessManager {
  AppContext *ctx;
  std::ifstream blFile; // Backlight file
  std::string subsystem;

  bool err;

public:
  BrightnessManager(AppContext *ctx);
  
  short getBrightness();
  bool setBrightness(short brightness);
  ~BrightnessManager();
};
