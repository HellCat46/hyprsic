#include "module.hpp"
#include <iomanip>
#include <sstream>

SysInfoModule::SysInfoModule(Stats* stats, Memory* memory, SysLoad* sysLoad, BatteryInfo* batteryInfo) : stat(stats), mem(memory), load(sysLoad), battery(batteryInfo) {

}

void SysInfoModule::setup(GtkWidget* gridBox){
    std::string txt = "";
 
    txt = "⬇" + stat->GetNetRx() + "⬆" + stat->GetNetTx();
    netWid = gtk_label_new(txt.c_str());

    txt = " " + stat->GetDiskAvail() + "/" + stat->GetDiskTotal();
    diskWid = gtk_label_new(txt.c_str());

    stat->UpdateData();

    txt = std::to_string(load->GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    loadWid = gtk_label_new(txt.c_str());

    txt = " " + Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(mem->GetTotRAM() * 1000, 2);
    memWid = gtk_label_new(txt.c_str());

    txt = " " + std::to_string(battery->getTotPercent()) + "%";
    batteryWid = gtk_label_new(txt.c_str());

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    timeWid = gtk_label_new(oss.str().c_str());

    gtk_grid_attach(GTK_GRID(gridBox), netWid, 0, 0, 1, 1);
    gtk_widget_set_hexpand(netWid, TRUE);
    gtk_grid_attach(GTK_GRID(gridBox), diskWid, 1, 0, 1, 1);
    gtk_widget_set_hexpand(diskWid, TRUE);
    gtk_grid_attach(GTK_GRID(gridBox), loadWid, 2, 0, 1, 1);
    gtk_widget_set_hexpand(loadWid, TRUE);
    gtk_grid_attach(GTK_GRID(gridBox), memWid, 3, 0, 1, 1);
    gtk_widget_set_hexpand(memWid, TRUE);
    gtk_grid_attach(GTK_GRID(gridBox), batteryWid, 4, 0, 1, 1);
    gtk_widget_set_hexpand(batteryWid, TRUE);
    gtk_grid_attach(GTK_GRID(gridBox), timeWid, 5, 0, 1, 1);
    gtk_widget_set_hexpand(timeWid, TRUE);
   
}

void SysInfoModule::update(){
    std::string txt = "";
    
    // Update Network Usage
    txt = "⬇" + stat->GetNetRx() + "⬆" + stat->GetNetTx();
    gtk_label_set_label(GTK_LABEL(netWid), txt.c_str());

    // Update Disk Usage
    txt = " " + stat->GetDiskAvail() + "/" + stat->GetDiskTotal();
    gtk_label_set_label(GTK_LABEL(diskWid), txt.c_str());

    // Update System Load
    txt = std::to_string(load->GetLoad(5));
    txt = " " + txt.substr(0, txt.find('.') + 3);
    gtk_label_set_label(GTK_LABEL(loadWid), txt.c_str());

    // Update memory
    txt = " " + Stats::ParseBytes(mem->GetUsedRAM() * 1000, 2) + "/" +
          Stats::ParseBytes(mem->GetTotRAM() * 1000, 2);
    gtk_label_set_label(GTK_LABEL(memWid), txt.c_str());

    // Update battery
    txt = " " + std::to_string(battery->getTotPercent()) + "%";
    gtk_label_set_label(GTK_LABEL(batteryWid), txt.c_str());

    // Update time
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    gtk_label_set_label(GTK_LABEL(timeWid), oss.str().c_str());

}