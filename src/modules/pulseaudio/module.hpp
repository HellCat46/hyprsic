#pragma once

#include "gdk-pixbuf/gdk-pixbuf.h"
#include "gtk/gtk.h"
#include "../../app/context.hpp"
#include "manager.hpp"

class PulseAudioModule {
  PulseAudioManager *paManager;
  AppContext *ctx;
  GtkWidget *menuWin;

  GtkWidget *inEvtBox;
  GtkWidget *barInIcon;
  GdkPixbuf *inMuteIcon;
  GdkPixbuf *inUnmuteIcon;

  GtkWidget *outEvtBox;
  GtkWidget *barOutIcon;
  GdkPixbuf *outMuteIcon;
  GdkPixbuf *outUnmuteIcon;

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

public:
  PulseAudioModule(PulseAudioManager *paManager, AppContext *ctx);
  void setup(GtkWidget *parent);
  void update();

  void updateControls(bool mute, const std::vector<uint32_t> &volume,
                             GtkWidget *icon, GtkWidget *scale);

  // Used for Both Volume and Mic controls along with the Opening Window
  static void handleIconClick(GtkWidget *widget, GdkEventButton *evtBtn,
                              gpointer data);
  static void handleChgVolume(GtkRange *range, GtkScrollType *scroll,
                              gdouble value, gpointer user_data);
  static void handleToggleMute(GtkWidget *widget, gpointer data);
  static void chgDevice(GtkComboBox *combo, gpointer data);
};
