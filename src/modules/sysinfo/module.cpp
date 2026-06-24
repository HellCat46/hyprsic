#include "header/module.hpp"
#include "gtkmm/widget.h"
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
    std::vector<std::reference_wrapper<Gtk::Widget>> &widgets) {
  stat->UpdateData();

  widgets.push_back(netWid);
  widgets.push_back(tempWid);

  diskIcon.set_from_icon_name("drive-harddisk-solidstate-symbolic");
  diskBox.append(diskIcon);
  diskBox.append(diskLbl);
  widgets.push_back(diskBox);
  
  widgets.push_back(loadWid);

  memIcon.set_from_icon_name("media-flash-symbolic");
  memBox.append(memIcon);
  memBox.append(memLbl);
  widgets.push_back(memBox);

  battBox.append(battIcon);
  battBox.append(battLbl);
  widgets.push_back(battBox);

  
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
  txt = stat->GetDiskAvail();
  diskLbl.set_label(txt);
  diskLbl.set_tooltip_markup("<b>Total:</b> " + stat->GetDiskTotal());

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
  txt = Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  memLbl.set_label(txt);
  tooltipTxt = "";
  txt = Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  tooltipTxt += "<b>Used RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotRAM() * 1000, 2);
  tooltipTxt += "<b>Total RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetUsedSwap() * 1000, 2);
  tooltipTxt += "<b>Used Swap:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotSwap() * 1000, 2);
  tooltipTxt += "<b>Total Swap:</b> " + txt;
  memBox.set_tooltip_markup(tooltipTxt);

  // Update battery
  updateBattery();

  // Update time
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  timeWid.set_label(oss.str());
}

void SysInfoModule::updateBattery() {
  BatteryStats battStats = battery->getBatteryStats();
  std::string txt = std::to_string(battStats.percent) + "%";
  battLbl.set_label(txt);

  // Update battery icon
  std::string iconStr = "battery-";
  if (battStats.percent < 10)
    iconStr += "caution-";
  else if (battStats.percent < 20)
    iconStr += "low-";
  else if (battStats.percent < 80)
    iconStr += "good-";
  else if (battStats.percent <= 100)
    iconStr += "full-";

  if(battery->isCharging())
    iconStr += "charging-";

  iconStr += "symbolic";
  battIcon.set_from_icon_name(iconStr);

  // Update battery tooltip
  std::string tooltipTxt = "<b>Charger:</b> ";
  if (battery->isCharging()) {
    tooltipTxt += "Charging";
    tooltipTxt += "\n<b>Time Till Full:</b> " +
                  HelperFunc::convertToTime(battStats.timeTillFull);
  } else {
    tooltipTxt += "Not Charging";
    tooltipTxt += "\n<b>Time Till Empty:</b> " +
                  HelperFunc::convertToTime(battStats.timeTillEmpty);
  }
  battBox.set_tooltip_markup(tooltipTxt);
}
