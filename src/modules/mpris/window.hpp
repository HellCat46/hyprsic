#pragma once

#include "../../app/context.hpp"
#include "manager.hpp"

class MprisWindow {
  MprisManager *manager;
  AppContext *ctx;

  GtkWidget *menuWin;
  GtkWidget *progTtl;
  GtkWidget *progLen;

  GtkWidget *progBarBox;
  GtkWidget *scaleMin;
  GtkWidget *scale;
  GtkWidget *scaleMax;

  GtkAdjustment *scaleAdj;

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

public:
  MprisWindow(AppContext *ctx, MprisManager *mprisMgr);
  void init();
  void update();

  bool isVisible() const;
  void chgVisibility(bool visible);

  static std::string timeToStr(uint64_t totalSeconds);
};
