#pragma once

#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

class MprisModule {
  MprisManager mprisInstance;
  LoggingManager *logger;

  GtkWidget *mainLabel;
  GtkWidget *popOverMenu;
  GtkWidget *progTitle;
  GtkWidget *progLength;
  GtkWidget *progScale;
  GtkAdjustment *progScaleAdj;

public:
  MprisModule(AppContext *ctx);

  void setup(GtkWidget *mainBox);
  void Update();

  static void showPopOverMenu(GtkWidget *widget, GdkEvent *e,
                              gpointer user_data);
  static void hidePopOverMenu(GtkWidget *widget, GdkEvent *e,
                              gpointer user_data);

  // Gtk Scale Signal Callbacks
  static gchar *handleFormatValue(GtkScale *scale, gdouble value,
                                  gpointer user_data);
  static gboolean handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                    gdouble value, gpointer user_data);

  // Track Control Buttons
  static void handlePlayPause(GtkWidget *widget, GdkEvent *e,
                              gpointer user_data);
  static void handleNextTrack(GtkWidget *widget, gpointer user_data);
  static void handlePrevTrack(GtkWidget *widget, gpointer user_data);
};
