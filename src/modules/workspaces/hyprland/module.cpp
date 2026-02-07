#include "module.hpp"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

HyprWSModule::HyprWSModule(AppContext *ctx, HyprWSManager *hyprInstance)
    : logger(&ctx->logger), hyprInstance(hyprInstance) {}

void HyprWSModule::setup(GtkWidget *mainGrid) {
  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_grid_attach(GTK_GRID(mainGrid), mainBox, 0, 0, 2, 1);
  gtk_widget_set_hexpand(mainBox, TRUE);

  wsWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_margin_start(wsWid, 15);
  gtk_box_pack_start(GTK_BOX(mainBox), wsWid, false, false, 0);

  spWSWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_margin_start(spWSWid, 15);
  gtk_box_pack_start(GTK_BOX(mainBox), spWSWid, false, false, 0);

  updateWorkspaces(hyprInstance, wsWid, spWSWid);

  hyprInstance->subscribe(HyprWSModule::updateWorkspaces, wsWid, spWSWid);
}

void HyprWSModule::updateWorkspaces(HyprWSManager *hyprInstance,
                                    GtkWidget *wsBox, GtkWidget *spWSBox) {

  if (!hyprInstance->GetWorkspaces()) {

    auto data = new UpdateWSData{hyprInstance, wsBox, spWSBox};
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, updateWorkspaceUI, data,
                    (GDestroyNotify) nullptr);
  }
}

gboolean HyprWSModule::updateWorkspaceUI(gpointer data) {
  UpdateWSData *upData = static_cast<UpdateWSData *>(data);
  std::string txt = "";

  GList *child = gtk_container_get_children(GTK_CONTAINER(upData->wsWid));
  for (GList *iter = child; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(child);

  child = gtk_container_get_children(GTK_CONTAINER(upData->spWSWid));
  for (GList *iter = child; iter != nullptr; iter = iter->next) {
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  }
  g_list_free(child);

  for (auto workspace : upData->wsInstance->workspaces) {
    txt = workspace.second.name;

    if (upData->wsInstance->activeWorkspaceId == workspace.first &&
        workspace.first >= 0) {
      txt = "[ " + txt + " ]";
    } else if (workspace.first < 0 && txt.length() > 0) {
      std::transform(txt.begin(), txt.begin() + 1, txt.begin(), ::toupper);
      txt = txt[0];
    }

    txt = "<b>" + txt + "</b>";

    GtkWidget *evtBox = gtk_event_box_new();
    GtkWidget *wsLabel = gtk_label_new(nullptr);
    gtk_label_set_markup(GTK_LABEL(wsLabel), txt.c_str());
    gtk_container_add(GTK_CONTAINER(evtBox), wsLabel);

    ChgWSArgs *args = g_new0(ChgWSArgs, 1);
    args->wsInstance = upData->wsInstance;
    args->wsId = workspace.first;

    if (workspace.first < 0) {
      gtk_widget_set_tooltip_markup(
          wsLabel, ("<b>" + workspace.second.name + "</b>").c_str());
      gtk_box_pack_start(GTK_BOX(upData->spWSWid), evtBox, false, false, 0);

      args->name = workspace.second.name;
      g_signal_connect_data(evtBox, "button-press-event",
                            G_CALLBACK(HyprWSModule::chgSPWS), args,
                            (GClosureNotify)g_free, (GConnectFlags)0);
    } else {
      gtk_box_pack_start(GTK_BOX(upData->wsWid), evtBox, false, false, 0);

      g_signal_connect_data(evtBox, "button-press-event",
                            G_CALLBACK(HyprWSModule::chgWS), args,
                            (GClosureNotify)g_free, (GConnectFlags)0);
    }
    gtk_widget_show_all(upData->wsWid);
    gtk_widget_show_all(upData->spWSWid);
  }

  delete upData;
  return G_SOURCE_REMOVE;
}

void HyprWSModule::chgWS(GtkWidget *widget, GdkEvent *e, gpointer user_data) {
  ChgWSArgs *args = static_cast<ChgWSArgs *>(user_data);

  if (args->wsInstance->SwitchToWS(args->wsInstance, args->wsId)) {
    std::cerr << "[Error] Failed to Switch Workspace" << std::endl;
  }
}

void HyprWSModule::chgSPWS(GtkWidget *widget, GdkEvent *e, gpointer user_data) {
  ChgWSArgs *args = static_cast<ChgWSArgs *>(user_data);

  if (args->wsInstance->SwitchSPWS(args->wsInstance, args->wsId, args->name)) {
    std::cerr << "[Error] Failed to Switch to Special Workspace" << std::endl;
  }
}
