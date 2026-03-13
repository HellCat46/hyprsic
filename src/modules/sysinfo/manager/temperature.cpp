#include "../header/manager/temperature.hpp"
#include "services/header/context.hpp"
#include <filesystem>

#define TAG "TemperatureManager"

TemperatureManager::TemperatureManager(AppContext *ctx) : ctx(ctx) {
  for (auto entry :
       std::filesystem::directory_iterator("/sys/class/thermal/")) {

    auto entryName = entry.path().filename().string();
    if (entryName.find("thermal_zone") == std::string::npos) {
      continue;
    }

    std::ifstream file(entry.path() / "type");
    if (!file.is_open()) {
      continue;
    }

    std::string type;
    std::getline(file, type);
    file.close();

    ctx->logger.LogInfo(TAG, "Found thermal sensor: " + entryName +
                                 " of type: " + type);
    if (type == "x86_pkg_temp" || type == "cpu-thermal" ||
        type.find("acpitz") != std::string::npos) {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::CPU});
    } else if (type.find("gpu") != std::string::npos) {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::GPU});
    } else if (type.find("wifi") != std::string::npos) {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::WIFI});
    } else if (type.find("nvme") != std::string::npos) {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::NVME_SSD});
    } else if (type.find("BATT") != std::string::npos ||
               type.find("battery") != std::string::npos) {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::BATTERY});
    } else if (type == "soc_thermal") {
      sensors.push_back(
          {std::ifstream(entry.path() / "temp"), 0, SensorType::SOC});
    }
  }

  ctx->logger.LogInfo(TAG, "TemperatureManager initialized with " +
                               std::to_string(sensors.size()) + " sensors.");
  update();
}

void TemperatureManager::update() {
  for (auto &sensor : sensors) {
    if (sensor.file.is_open()) {
      sensor.file.seekg(0);
      sensor.file >> sensor.temp;
    }
  }
}

short TemperatureManager::getSensorTemp(SensorType type) {
  for (const auto &sensor : sensors) {
    if (sensor.type == type) {
      return sensor.temp / 1000;
    }
  }
  return 0;
}

TemperatureManager::~TemperatureManager() {
  for (auto &sensor : sensors) {
    if (sensor.file.is_open()) {
      sensor.file.close();
    }
  }
}
