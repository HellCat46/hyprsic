#pragma once

#include "glib.h"
#include <gtk-layer-shell.h>
#include <gtk/gtk.h>

#include "../collectors/stats.hpp"
#include "../collectors/battery.hpp"
#include "../collectors/disk.hpp"
#include "../collectors/memory.hpp"
#include "../collectors/network.hpp"
#include "../collectors/playing_now.hpp"
#include "../collectors/sys_load.hpp"
#include "../collectors/workspaces/hyprland.hpp"

class MainWindow {
  GtkApplication *app = nullptr;
  
  Stats stat;
  GtkWidget* netWid;
  GtkWidget* diskWid;
  
  SysLoad load;
  GtkWidget* loadWid;
  
  Memory mem;
  GtkWidget* memWid;
  
  BatteryInfo battery;
  GtkWidget* batteryWid;
  
  GtkWidget* timeWid;
    
  PlayingNow playing;
  HyprWorkspaces hyprWS;

public:
  MainWindow();
  void RunApp();
  static gboolean UpdateData(gpointer data);
  static void activate(GtkApplication *app, gpointer user_data);
};
