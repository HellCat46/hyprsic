#pragma once

#include "gtk/gtk.h"
#include "manager.hpp"

class PulseAudioModule {
  PulseAudioManager *paManager;
  LoggingManager *logger;
  GtkWidget *audioWin;

  GtkWidget *outMuteBtn;
  GtkWidget *outScale;
  GtkWidget *outDropdown;
  GtkListStore *outStore;

  GtkWidget *inMuteBtn;
  GtkWidget *inScale;
  GtkWidget *inDropdown;
  GtkListStore *inStore;

public:
  PulseAudioModule(PulseAudioManager *paManager, LoggingManager *logMgr);
  void setup(GtkWidget *parent);
  void update();
  
  static void updateControls(bool mute, const std::vector<uint32_t> &volume, GtkWidget* muteBtn,
                                        GtkWidget *scale);

  static void handleOpenWindow(GtkWidget *widget, GdkEventButton *evtBtn,
                               gpointer data);
  static void handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                              gdouble value, gpointer user_data);
  static void handleToggleMute(GtkWidget *widget, gpointer data);
  static void chgeDevice(GtkComboBox *combo, gpointer data);
};
