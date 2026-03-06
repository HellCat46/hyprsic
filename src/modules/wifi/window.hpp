#pragma once

#include "services/header/context.hpp"
#include "gtk/gtk.h"
#include "manager.hpp"
class WifiWindow {
    AppContext* ctx;
    WifiManager* manager;
    GtkWidget* mainBox;
    
    GtkWidget* scanBtn;
    GtkWidget* powerBtn;
    
    GtkWidget* connDevBox;
    GtkWidget* connDevIBox;
    GtkWidget* connDeviceName;
    
    GtkWidget* devBox;
    GtkWidget* devListScrlBox;
    
    static void handleScan(GtkWidget *widget, gpointer user_data);
    static void handleDisconnect(GtkWidget *widget, gpointer user_data);
    static void handleForget(GtkWidget *widget, gpointer user_data);
    
    public:
    WifiWindow(AppContext* context, WifiManager* manager);
    void init();
    void update();
};

