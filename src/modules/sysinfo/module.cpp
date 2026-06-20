#include "header/module.hpp"
#include "services/header/context.hpp"
#include "utils/helper_func.hpp"
#include <functional>
#include <iomanip>
#include <sstream>
#include <string>

SysInfoModule::SysInfoModule(AppContext *ctx, Stats *stats, Memory *memory,
                             SysLoad *sysLoad, BatteryInfo *batteryInfo,
                             TemperatureManager *tempMgr)
    : ctx(ctx), stat(stats), mem(memory), load(sysLoad), battery(batteryInfo),
      tempManager(tempMgr) {}

void SysInfoModule::setup(
    std::vector<std::reference_wrapper<Gtk::Label>> &widgets) {
  stat->UpdateData();

  widgets.push_back(netWid);
  widgets.push_back(tempWid);
  widgets.push_back(diskWid);
  widgets.push_back(loadWid);
  widgets.push_back(memWid);
  widgets.push_back(batteryWid);
  widgets.push_back(timeWid);

  update();
}

void SysInfoModule::update() {
  std::string txt = "", tooltipTxt = "";

  // Update Network Usage
  txt = "⬇" + stat->GetNetRx() + "⬆" + stat->GetNetTx();
  netWid.set_label(txt);
  netWid.set_tooltip_markup(stat->GetIfaces());

  // Update Temperature
  txt = std::to_string(tempManager->getSensorTemp(SensorType::CPU)) + "°C";
  tempWid.set_label(txt);

  // Update Disk Usage
  txt = " " + stat->GetDiskAvail();
  diskWid.set_label(txt);
  diskWid.set_tooltip_markup("<b>Total:</b> " + stat->GetDiskTotal());

  // Update System Load
  txt = std::to_string(load->GetLoad(5));
  txt = " " + txt.substr(0, txt.find('.') + 3);
  loadWid.set_label(txt);
  
  txt = std::to_string(load->GetLoad(1));
  tooltipTxt = "<b>1 Min:</b> " + txt.substr(0, txt.find('.') + 3) + "\n";
  txt = std::to_string(load->GetLoad(5));
  tooltipTxt += "<b>5 Min:</b> " + txt.substr(0, txt.find('.') + 3) + "\n";
  txt = std::to_string(load->GetLoad(15));
  tooltipTxt += "<b>15 Min:</b> " + txt.substr(0, txt.find('.') + 3);
  loadWid.set_tooltip_markup(tooltipTxt);

  // Update memory
  txt = " " + Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  memWid.set_label(txt);
  tooltipTxt = "";
  txt = Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  tooltipTxt += "<b>Used RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotRAM() * 1000, 2);
  tooltipTxt += "<b>Total RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetUsedSwap() * 1000, 2);
  tooltipTxt += "<b>Used Swap:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotSwap() * 1000, 2);
  tooltipTxt += "<b>Total Swap:</b> " + txt;
  memWid.set_tooltip_markup(tooltipTxt);

  // Update battery
  BatteryStats battStats = battery->getBatteryStats();
  txt = " " + std::to_string(battStats.percent) + "%";
  batteryWid.set_label(txt);
  tooltipTxt = "<b>Charger:</b> ";
  if (battery->isCharging()) {
    tooltipTxt += "Charging";
    tooltipTxt += "\n<b>Time Till Full:</b> " +
                  HelperFunc::convertToTime(battStats.timeTillFull);
  } else {
    tooltipTxt += "Not Charging";
    tooltipTxt += "\n<b>Time Till Empty:</b> " +
                  HelperFunc::convertToTime(battStats.timeTillEmpty);
  }
  batteryWid.set_tooltip_markup(tooltipTxt);

  // Update time
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  timeWid.set_label(oss.str());
}
