#pragma once

#include "gtk/gtk.h"
#include "../../app/context.hpp"
#include "manager.hpp"
#include "window.hpp"
#include <vector>

class PulseAudioModule {
  PulseAudioManager *manager;
  PulseAudioWindow *window;
  AppContext *ctx;
  GtkWidget *menuBox;

  GtkWidget *inEvtBox;
  GtkWidget *barInIcon;

  GtkWidget *outEvtBox;
  GtkWidget *barOutIcon;

  bool setupComp;
  
  // Used for Both Volume and Mic controls along with the Opening Window
  static void handleIconClick(GtkWidget *widget, GdkEventButton *evtBtn,
                              gpointer data);
  

public:
  PulseAudioModule(PulseAudioManager *paManager, AppContext *ctx, PulseAudioWindow *window);
  std::vector<GtkWidget*> setup();
  
  void update();
  void updateControls(bool mute,
                      GtkWidget *icon);
};
