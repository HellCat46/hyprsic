#include "module.hpp"
#include "gdk/gdk.h"
#include "glib-object.h"
#include "glibconfig.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <cstdint>

#define TAG "PulseAudioModule"

PulseAudioModule::PulseAudioModule(PulseAudioManager *paMgr,
                                   LoggingManager *logMgr)
    : logger(logMgr), paManager(paMgr) {}

void PulseAudioModule::setup(GtkWidget *parent) {
  GtkWidget *audioEvtBox = gtk_event_box_new();
  GtkWidget *audio = gtk_label_new("A");
  gtk_container_add(GTK_CONTAINER(audioEvtBox), audio);
  gtk_grid_attach(GTK_GRID(parent), audioEvtBox, 10, 0, 1, 1);

  audioWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_layer_init_for_window(GTK_WINDOW(audioWin));
  gtk_layer_set_layer(GTK_WINDOW(audioWin), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_anchor(GTK_WINDOW(audioWin), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(audioWin), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);

  GtkWidget *mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_container_add(GTK_CONTAINER(audioWin), mainBox);
  gtk_widget_set_margin_start(mainBox, 15);
  gtk_widget_set_margin_end(mainBox, 15);
  gtk_widget_set_margin_top(mainBox, 15);
  gtk_widget_set_margin_bottom(mainBox, 15);

  // Output Device Controls
  GtkWidget *outTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(outTitle), "<b>Output Device:</b>");
  gtk_widget_set_halign(outTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(mainBox), outTitle, FALSE, FALSE, 0);

  GtkWidget *outBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), outBox, FALSE, FALSE, 0);

  outMuteBtn = gtk_button_new_with_label("M");
  gtk_box_pack_start(GTK_BOX(outBox), outMuteBtn, FALSE, FALSE, 0);
  g_signal_connect(outMuteBtn, "clicked", G_CALLBACK(handleToggleMute), this);

  outScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_box_pack_start(GTK_BOX(outBox), outScale, TRUE, TRUE, 0);
  g_signal_connect(outScale, "change-value", G_CALLBACK(handleChgVolume), this);

  outStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  outDropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(outStore));
  g_signal_connect(outDropdown, "changed", G_CALLBACK(chgeDevice), this);
  gtk_box_pack_start(GTK_BOX(mainBox), outDropdown, FALSE, FALSE, 0);

  GtkCellRenderer *outRenderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(outDropdown), outRenderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(outDropdown), outRenderer,
                                 "text", 1, nullptr);

  // Input Device Controls
  GtkWidget *inTitle = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(inTitle), "<b>Input Device:</b>");
  gtk_widget_set_halign(inTitle, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(mainBox), inTitle, FALSE, FALSE, 0);
  gtk_widget_set_margin_top(inTitle, 20);

  GtkWidget *inBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(mainBox), inBox, TRUE, TRUE, 0);

  inMuteBtn = gtk_button_new_with_label("M");
  gtk_box_pack_start(GTK_BOX(inBox), inMuteBtn, FALSE, FALSE, 0);
  g_signal_connect(outMuteBtn, "clicked", G_CALLBACK(handleToggleMute), this);

  inScale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
  gtk_box_pack_start(GTK_BOX(inBox), inScale, TRUE, TRUE, 0);
  g_signal_connect(inScale, "change-value", G_CALLBACK(handleChgVolume), this);

  inStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
  inDropdown = gtk_combo_box_new_with_model(GTK_TREE_MODEL(inStore));
  g_signal_connect(outDropdown, "changed", G_CALLBACK(chgeDevice), this);
  gtk_box_pack_start(GTK_BOX(mainBox), inDropdown, FALSE, FALSE, 0);

  GtkCellRenderer *inRenderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(inDropdown), inRenderer, TRUE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(inDropdown), inRenderer,
                                 "text", 1, nullptr);

  gtk_widget_set_size_request(audioWin, 400, -1);
  gtk_widget_show_all(audioEvtBox);
  gtk_widget_show_all(mainBox);
  g_signal_connect(audioEvtBox, "button-press-event",
                   G_CALLBACK(handleOpenWindow), this);

  update();
}

void PulseAudioModule::update() {
  // Adding Items to Output Selector
  gtk_list_store_clear(outStore);
  GtkTreeIter iter, activeIter;
  bool foundActive = false;
  for (const auto &[devName, devInfo] : paManager->outDevs) {
    gtk_list_store_append(outStore, &iter);

    gtk_list_store_set(outStore, &iter, 0, devName.c_str(), 1,
                       devInfo.description.c_str(), -1);

    if (devName == paManager->defOutput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Output Device for Control Widgets
      updateControls(devInfo.mute, devInfo.volume, outMuteBtn, outScale);
    }
  }
  if (foundActive) {
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(outDropdown), &activeIter);
  }

  // Adding Items to Input Selector
  gtk_list_store_clear(inStore);
  for (const auto &[devName, devInfo] : paManager->inDevs) {
    gtk_list_store_append(inStore, &iter);
    
    gtk_list_store_set(inStore, &iter, 0, devName.c_str(), 1,
                       devInfo.description.c_str(), -1);

    if (devName == paManager->defInput) {
      activeIter = iter;
      foundActive = true;

      // Using Default Input Device for Control Widgets
      updateControls(devInfo.mute, devInfo.volume, inMuteBtn, inScale);
    }
  }

  if (foundActive) {
    gtk_combo_box_set_active_iter(GTK_COMBO_BOX(inDropdown), &activeIter);
  }
}

void PulseAudioModule::updateControls(bool mute,
                                      const std::vector<uint32_t> &volume,
                                      GtkWidget *muteBtn, GtkWidget *scale) {
  gtk_button_set_label(GTK_BUTTON(muteBtn), mute ? "U" : "M");

  uint32_t avgVol = 0;
  for (const auto &vol : volume) {
    avgVol += vol;
  }
  avgVol /= volume.size();
  gtk_range_set_value(GTK_RANGE(scale),
                      (uint32_t)(((float)avgVol / 65535) * 100));
}

void PulseAudioModule::handleOpenWindow(GtkWidget *widget,
                                        GdkEventButton *evtBtn, gpointer data) {
  PulseAudioModule *self = static_cast<PulseAudioModule *>(data);

  if (gtk_widget_get_visible(self->audioWin)) {
    gtk_widget_hide(self->audioWin);
    return;
  }

  gtk_widget_show(self->audioWin);
}

void PulseAudioModule::handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                                       gdouble value, gpointer data) {
  PulseAudioModule *self = static_cast<PulseAudioModule *>(data);

  if (range == GTK_RANGE(self->outScale)) {
    uint32_t volume = (uint32_t)gtk_range_get_value(range);
    self->paManager->setVolume(self->paManager->defOutput, true, volume);
  } else if (range == GTK_RANGE(self->inScale)) {
    uint32_t volume = (uint32_t)gtk_range_get_value(range);
    self->paManager->setVolume(self->paManager->defInput, false, volume);
  }
}

void PulseAudioModule::handleToggleMute(GtkWidget *widget, gpointer data) {
  PulseAudioModule *self = static_cast<PulseAudioModule *>(data);

  if (widget == self->outMuteBtn) {
    short res = self->paManager->toggleMute(self->paManager->defOutput, true);
    if (res != -1) {
      gtk_button_set_label(GTK_BUTTON(self->outMuteBtn), res ? "U" : "M");
    }
  } else if (widget == self->inMuteBtn) {
    short res = self->paManager->toggleMute(self->paManager->defInput, false);
    if (res != -1) {
      gtk_button_set_label(GTK_BUTTON(self->inMuteBtn), res ? "U" : "M");
    }
  }
}

void PulseAudioModule::chgeDevice(GtkComboBox *combo, gpointer data) {
  PulseAudioModule *self = static_cast<PulseAudioModule *>(data);

  GtkTreeIter iter;
  if (gtk_combo_box_get_active_iter(combo, &iter)) {
    gchar *devName;
    gtk_tree_model_get(GTK_TREE_MODEL(gtk_combo_box_get_model(combo)), &iter, 0,
                       &devName, -1);

    self->paManager->updateDefDevice(std::string(devName),
                                     combo == GTK_COMBO_BOX(self->outDropdown));

    g_free(devName);
  }
}
