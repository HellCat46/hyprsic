#pragma once

#include "gdk/gdk.h"
#include "glib.h"
#include "gtk/gtk.h"
#include "manager.hpp"

class MprisModule {
  MprisManager* mprisInstance;
  LoggingManager *logger;

  GtkWidget *mainLabel;
  GtkWidget *menuWindow;
  GtkWidget *progTitle;
  GtkWidget *progLength;
  
  GtkWidget* progBarBox;
  GtkWidget* progScaleMin;
  GtkWidget* progScale;
  GtkWidget* progScaleMax;
  
  GtkAdjustment *progScaleAdj;

public:
  MprisModule(AppContext *ctx, MprisManager* mprisMgr);

  void setup(GtkWidget *mainBox);
  void Update();

  static void chgVisibilityMenu(GtkWidget *widget, GdkEvent *e,
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
  
  static std::string timeToStr(uint64_t totalSeconds);
};
