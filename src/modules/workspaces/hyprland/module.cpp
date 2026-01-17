#include "module.hpp"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <functional>
#include <iostream>

HyprWSModule::HyprWSModule(AppContext *ctx, HyprWSManager *hyprInstance) {
    this->hyprInstance = hyprInstance;
  logger = &ctx->logging;
}

void HyprWSModule::setup(GtkWidget *main_box) {
  SectionWid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
  gtk_widget_set_margin_start(SectionWid, 15);
  gtk_grid_attach(GTK_GRID(main_box), SectionWid, 0, 0, 2, 1);
  gtk_widget_set_hexpand(SectionWid, TRUE);

  updateWorkspaces(hyprInstance, SectionWid);

  hyprInstance->subscribe(HyprWSModule::updateWorkspaces, SectionWid);
}

void HyprWSModule::updateWorkspaces(HyprWSManager *hyprInstance,
                                    GtkWidget *SectionWid) {

  if (!hyprInstance->GetWorkspaces()) {
    auto updateLogic = [SectionWid, hyprInstance]() {
      std::string txt = "";

      GList *child = gtk_container_get_children(GTK_CONTAINER(SectionWid));
      for (GList *iter = child; iter != nullptr; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
      }
      g_list_free(child);

      for (auto workspace : hyprInstance->workspaces) {
        txt = std::to_string(workspace.first) + " ";

        if (hyprInstance->activeWorkspaceId == workspace.first) {
          txt = "[ " + txt + "]";
        }

        txt = "<b>" + txt + "</b>";

        GtkWidget *evtBox = gtk_event_box_new();
        GtkWidget *wsLabel = gtk_label_new(nullptr);
        gtk_label_set_markup(GTK_LABEL(wsLabel), txt.c_str());

        gtk_container_add(GTK_CONTAINER(evtBox), wsLabel);
        gtk_box_pack_start(GTK_BOX(SectionWid), evtBox, FALSE, FALSE, 0);

        ChgWSArgs *args = g_new0(ChgWSArgs, 1);
        args->wsInstance = hyprInstance;
        args->wsId = workspace.first;

        g_signal_connect_data(evtBox, "button-press-event",
                              G_CALLBACK(HyprWSModule::chgWorkspace), args,
                              (GClosureNotify)g_free, (GConnectFlags)0);
      }
      gtk_widget_show_all(SectionWid);

      return G_SOURCE_REMOVE;
    };

    auto *callback = new std::function<gboolean()>(std::move(updateLogic));

    g_idle_add_full(
        G_PRIORITY_DEFAULT_IDLE,
        [](gpointer data) -> gboolean {
          auto *func = static_cast<std::function<gboolean()> *>(data);

          return (*func)();
        },
        callback,
        [](gpointer data) {
          delete static_cast<std::function<gboolean()> *>(data);
        });
  }
}

void HyprWSModule::chgWorkspace(GtkWidget *widget, GdkEvent *e,
                                gpointer user_data) {
  ChgWSArgs *args = static_cast<ChgWSArgs *>(user_data);

  if (args->wsInstance->SwitchToWorkspace(args->wsInstance, args->wsId) != 0) {
    std::cerr << "[Error] Failed to Switch Workspace" << std::endl;
  }
}
