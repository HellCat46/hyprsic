#include "module.hpp"
#include "gdk/gdk.h"
#include "glib-object.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <algorithm>
#include <functional>
#include <string>

HyprWSModule::HyprWSModule(AppContext *ctx, HyprWSManager *hyprInstance)
    : logger(&ctx->logger), hyprInstance(hyprInstance) {}

void HyprWSModule::setup(GtkWidget *mainGrid, unsigned char monitorId) {
  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_grid_attach(GTK_GRID(mainGrid), mainBox, 0, 0, 2, 1);
  gtk_widget_set_hexpand(mainBox, TRUE);

  wsWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  GtkWidget *wdEvtBox = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(wdEvtBox), wsWid);
  gtk_widget_set_margin_start(wdEvtBox, 15);
  gtk_box_pack_start(GTK_BOX(mainBox), wdEvtBox, false, false, 0);
  gtk_widget_add_events(wdEvtBox, GDK_SCROLL_MASK);
  g_signal_connect(wdEvtBox, "scroll-event",
                   G_CALLBACK(HyprWSModule::handleWSScroll), this);

  spWSWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_margin_start(spWSWid, 15);
  gtk_box_pack_start(GTK_BOX(mainBox), spWSWid, false, false, 0);

  this->monitorId = monitorId;
  updateWorkspaces(hyprInstance, wsWid, spWSWid, monitorId);
  hyprInstance->subscribe(HyprWSModule::updateWorkspaces, wsWid, spWSWid,
                          monitorId);
}

void HyprWSModule::updateWorkspaces(HyprWSManager *hyprInstance,
                                    GtkWidget *wsBox, GtkWidget *spWSBox,
                                    unsigned char monitorId) {

  if (!hyprInstance->GetWorkspaces()) {

    auto data = new UpdateWSData{hyprInstance, wsBox, spWSBox, monitorId};
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

    if (upData->monitorId != workspace.second.monitorId)
      continue;

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

  args->wsInstance->SwitchToWS(args->wsId);
}

void HyprWSModule::handleWSScroll(GtkWidget *widget, GdkEventScroll *e,
                                  gpointer user_data) {
  HyprWSModule *self = static_cast<HyprWSModule *>(user_data);

  if (e->direction == GDK_SCROLL_UP) {
    self->hyprInstance->MoveToWS(self->hyprInstance->activeWorkspaceId - 1,
                                 self->monitorId, false);
  } else if (e->direction == GDK_SCROLL_DOWN) {
    self->hyprInstance->MoveToWS(self->hyprInstance->activeWorkspaceId + 1,
                                 self->monitorId, true);
  }
}

void HyprWSModule::chgSPWS(GtkWidget *widget, GdkEvent *e, gpointer user_data) {
  ChgWSArgs *args = static_cast<ChgWSArgs *>(user_data);
  args->wsInstance->SwitchSPWS(args->wsId, args->name);
}
