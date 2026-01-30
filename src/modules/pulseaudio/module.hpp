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

  GtkWidget *inMuteBtn;
  GtkWidget *inScale;
  GtkWidget *inDropdown;

public:
  PulseAudioModule(PulseAudioManager *paManager, LoggingManager *logMgr);
  void setup(GtkWidget *parent);
  void update();

  static void handleOpenWindow(GtkWidget *widget, GdkEventButton *evtBtn,
                               gpointer data);
  static void handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                              gdouble value, gpointer user_data);
  static void handleToggleMute(GtkWidget *widget,
                               gpointer data);
};
