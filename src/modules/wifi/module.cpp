#include "module.hpp"
#include "glib-object.h"
#include "gtk/gtk.h"
#include "modules/wifi/manager.hpp"
#include "modules/wifi/window.hpp"


WifiModule::WifiModule(AppContext* ctx, WifiManager* mgr, WifiWindow* window) : ctx(ctx), manager(mgr), window(window){
    
}


GtkWidget* WifiModule::setup(){
    GtkWidget* evtBox = gtk_event_box_new();
    g_signal_connect(evtBox, "button-press-event", G_CALLBACK(openWindow), this);
    
    mainLbl = gtk_label_new("Not Connected");
    gtk_container_add(GTK_CONTAINER(evtBox), mainLbl);
    
    return evtBox;
}

void WifiModule::update(){
    auto conn = manager->ConnectedDevice();
    if(conn.connected){
        gtk_label_set_text(GTK_LABEL(mainLbl), conn.ssid.c_str());
    } else {
        gtk_label_set_text(GTK_LABEL(mainLbl), "Not Connected");
    }
}
void WifiModule::openWindow([[maybe_unused]] GtkWidget* widget,[[maybe_unused]] GdkEvent *e, gpointer user_data) {
    WifiModule *self = static_cast<WifiModule *>(user_data);
    self->ctx->showCtrlWindow("wifi", 400, 400);
}
