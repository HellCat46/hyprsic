#pragma once

#include "services/header/context.hpp"
#include <fstream>
#include <vector>

enum class SensorType { CPU, GPU, WIFI, NVME_SSD, BATTERY, SOC };

struct Sensor {
  std::ifstream file;
  int temp;
  SensorType type;
};

class TemperatureManager {
  AppContext *ctx;
  std::vector<Sensor> sensors;

public:
  TemperatureManager(AppContext *ctx);
  ~TemperatureManager();

  void update();
  short getSensorTemp(SensorType type);
};
