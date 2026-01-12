#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"

class MprisModule {
    MprisManager mprisInstance;
    LoggingManager* logger;
    GtkWidget* mainLabel;
    
public:
    MprisModule(AppContext* ctx);
    
    void setup(GtkWidget* mainBox);
    void Update();
    
    static void handlePlayPause(GtkWidget* widget, GdkEvent* e, gpointer user_data);
};