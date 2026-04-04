#pragma once

#include "manager.hpp"
#include "services/header/comm_bus.hpp"
#include "services/header/context.hpp"

class PulseAudioWindow {
  AppContext *ctx;
  PulseAudioManager *manager;
  CommunicationBus *commBus;

  GtkWidget *outMuteBtn;
  GtkWidget *outIcon;
  GtkWidget *outScale;
  GtkWidget *outDropdown;
  GtkListStore *outStore;

  GtkWidget *inMuteBtn;
  GtkWidget *inIcon;
  GtkWidget *inScale;
  GtkWidget *inDropdown;
  GtkListStore *inStore;

  void updateControls(bool mute, const std::vector<uint32_t> &volume,
                      GtkWidget *icon, GtkWidget *scale);

  static void chgDevice(GtkComboBox *combo, gpointer data);
  static void handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                              gdouble value, gpointer user_data);
  static void handleToggleMute(GtkWidget *widget, gpointer data);

public:
  GdkPixbuf *outMuteIcon;
  GdkPixbuf *outUnmuteIcon;
  GdkPixbuf *inMuteIcon;
  GdkPixbuf *inUnmuteIcon;

  PulseAudioWindow(AppContext *ctx, CommunicationBus *commBus,
                   PulseAudioManager *manager);
  void setupIcons();
  void init();
  void update();

  void toggleMute(GtkWidget *widget, gpointer data, bool isOutput);
};
