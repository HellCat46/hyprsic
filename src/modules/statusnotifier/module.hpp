#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"
#include <cstdint>
#include <string>


struct MenuActionArgs {
    LoggingManager *logger;
  StatusNotifierManager *snManager;
  std::string itemId;
  SNIApp sniApp;
};

struct EvtBtnPressArgs {
    LoggingManager *logger;
  StatusNotifierManager *snManager;
  std::string itemId;
  SNIApp sniApp;
  uint32_t evtIdx;
};

class StatusNotifierModule {
  StatusNotifierManager *snManager;
  LoggingManager *logger;
  GtkWidget *sniBox;
  std::map<std::string, SNIApp> sniApps;
  
  
  public:
    StatusNotifierModule(AppContext *ctx, StatusNotifierManager *snManagerInstance);
    
    void setup(GtkWidget *box);
    void update();
    static void remove(std::string servicePath, std::map<std::string, SNIApp>* sniApps, GtkWidget* sniBox);
    
    static void handleContextMenuOpen(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
    static void handleEvtButtonPress(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
};