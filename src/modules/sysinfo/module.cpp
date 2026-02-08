#include "module.hpp"
#include "gtk/gtk.h"
#include "manager/battery.hpp"
#include "../../utils/helper_func.hpp"
#include <iomanip>
#include <sstream>
#include <string>

SysInfoModule::SysInfoModule(Stats *stats, Memory *memory, SysLoad *sysLoad,
                             BatteryInfo *batteryInfo)
    : stat(stats), mem(memory), load(sysLoad), battery(batteryInfo) {}

void SysInfoModule::setup(GtkWidget *gridBox) {

  stat->UpdateData();
  std::string txt = "";

  netWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), netWid, 1, 0, 1, 1);

  diskWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), diskWid, 2, 0, 1, 1);

  loadWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), loadWid, 3, 0, 1, 1);

  memWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), memWid, 4, 0, 1, 1);

  batteryWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), batteryWid, 5, 0, 1, 1);

  timeWid = gtk_label_new(nullptr);
  gtk_grid_attach(GTK_GRID(gridBox), timeWid, 6, 0, 1, 1);

  update();
}

void SysInfoModule::update() {
  std::string txt = "", tooltipTxt = "";

  // Update Network Usage
  txt = "⬇" + stat->GetNetRx() + "⬆" + stat->GetNetTx();
  gtk_label_set_label(GTK_LABEL(netWid), txt.c_str());
  gtk_widget_set_tooltip_markup(netWid, stat->GetIfaces().c_str());

  // Update Disk Usage
  txt = " " + stat->GetDiskAvail();
  gtk_label_set_label(GTK_LABEL(diskWid), txt.c_str());
  gtk_widget_set_tooltip_markup(
      diskWid, ("<b>Total:</b> " + stat->GetDiskTotal()).c_str());

  // Update System Load
  txt = std::to_string(load->GetLoad(5));
  txt = " " + txt.substr(0, txt.find('.') + 3);
  gtk_label_set_label(GTK_LABEL(loadWid), txt.c_str());
  txt = std::to_string(load->GetLoad(1));
  tooltipTxt = "<b>1 Min:</b> " + txt.substr(0, txt.find('.') + 3) + "\n";
  txt = std::to_string(load->GetLoad(5));
  tooltipTxt += "<b>5 Min:</b> " + txt.substr(0, txt.find('.') + 3) + "\n";
  txt = std::to_string(load->GetLoad(15));
  tooltipTxt += "<b>15 Min:</b> " + txt.substr(0, txt.find('.') + 3);
  gtk_widget_set_tooltip_markup(loadWid, tooltipTxt.c_str());

  // Update memory
  txt = " " + Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  gtk_label_set_label(GTK_LABEL(memWid), txt.c_str());
  tooltipTxt = "";
  txt = Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2);
  tooltipTxt += "<b>Used RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotRAM() * 1000, 2);
  tooltipTxt += "<b>Total RAM:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetUsedSwap() * 1000, 2);
  tooltipTxt += "<b>Used Swap:</b> " + txt + "\n";
  txt = Stats::ParseBytes(mem->GetTotSwap() * 1000, 2);
  tooltipTxt += "<b>Total Swap:</b> " + txt;
  gtk_widget_set_tooltip_markup(memWid, tooltipTxt.c_str());

  // Update battery
  BatteryStats battStats = battery->getBatteryStats();
  txt = " " + std::to_string(battStats.percent) + "%";
  gtk_label_set_label(GTK_LABEL(batteryWid), txt.c_str());
  tooltipTxt = "<b>Charger:</b> ";
  if(battery->isCharging()){
    tooltipTxt += "Charging";
    tooltipTxt += "\n<b>Time Till Full:</b> " + HelperFunc::convertToTime(battStats.timeTillFull);
  } else {
    tooltipTxt += "Not Charging";
    tooltipTxt += "\n<b>Time Till Empty:</b> " + HelperFunc::convertToTime(battStats.timeTillEmpty);
  }
  gtk_widget_set_tooltip_markup(batteryWid, tooltipTxt.c_str());

  // Update time
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  std::ostringstream oss;
  oss << std::put_time(&tm, "%H:%M:%S");
  gtk_label_set_label(GTK_LABEL(timeWid), oss.str().c_str());
}
