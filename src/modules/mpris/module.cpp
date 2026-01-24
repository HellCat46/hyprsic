#include "module.hpp"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk-layer-shell.h"
#include "gtk/gtk.h"
#include "manager.hpp"
#include <cerrno>
#include <cstdint>
#include <string>

#define TAG "MprisModule"

MprisModule::MprisModule(AppContext *ctx, MprisManager *mprisMgr) {
  mprisInstance = mprisMgr;
  logger = &ctx->logging;
}

void MprisModule::setup(GtkWidget *mainBox) {

  mainLabel = gtk_label_new(nullptr);
  GtkWidget *labelAction = gtk_event_box_new();
  g_signal_connect(labelAction, "button-press-event",
                   G_CALLBACK(MprisModule::chgVisibilityMenu), this);

  gtk_container_add(GTK_CONTAINER(labelAction), mainLabel);
  gtk_grid_attach(GTK_GRID(mainBox), labelAction, 2, 0, 1, 1);
  gtk_widget_set_hexpand(labelAction, FALSE);

  // Popover Menu
  menuWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_layer_init_for_window(GTK_WINDOW(menuWindow));
  gtk_layer_set_layer(GTK_WINDOW(menuWindow), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_namespace(GTK_WINDOW(menuWindow), "popup");
  gtk_layer_set_anchor(GTK_WINDOW(menuWindow), GTK_LAYER_SHELL_EDGE_BOTTOM,
                       TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWindow), GTK_LAYER_SHELL_EDGE_RIGHT,
                       FALSE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWindow), GTK_LAYER_SHELL_EDGE_LEFT,
                       FALSE);
  gtk_widget_set_size_request(menuWindow, 500, -1);

  GtkWidget *progBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(menuWindow), progBox);
  gtk_widget_set_margin_bottom(progBox, 10);
  gtk_widget_set_margin_top(progBox, 10);
  gtk_widget_set_margin_start(progBox, 10);
  gtk_widget_set_margin_end(progBox, 10);

  GtkWidget *winTitle = gtk_label_new(nullptr);
  gtk_box_pack_start(GTK_BOX(progBox), winTitle, FALSE, FALSE, 0);
  gtk_label_set_markup(GTK_LABEL(winTitle),
                       "<b>Now Playing - Player Control</b>");
  gtk_widget_set_margin_bottom(winTitle, 20);

  GtkWidget *titleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(progBox), titleBox, FALSE, FALSE, 0);

  // Prev Button
  GtkWidget *titlePrev = gtk_button_new_with_label("<");
  gtk_box_pack_start(GTK_BOX(titleBox), titlePrev, FALSE, FALSE, 0);
  g_signal_connect(titlePrev, "clicked",
                   G_CALLBACK(MprisModule::handlePrevTrack), this);

  GtkWidget *progEventListener = gtk_event_box_new();
  progTitle = gtk_label_new(nullptr);
  gtk_widget_set_halign(progTitle, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(progTitle, TRUE);
  gtk_container_add(GTK_CONTAINER(progEventListener), progTitle);
  gtk_box_pack_start(GTK_BOX(titleBox), progEventListener, FALSE, FALSE, 0);
  g_signal_connect(progEventListener, "button-press-event",
                   G_CALLBACK(MprisModule::handlePlayPause), mprisInstance);

  // Next Button
  GtkWidget *titleNext = gtk_button_new_with_label(">");
  gtk_box_pack_end(GTK_BOX(titleBox), titleNext, FALSE, FALSE, 0);
  g_signal_connect(titleNext, "clicked",
                   G_CALLBACK(MprisModule::handleNextTrack), this);

  // Scale for Track Progress
  progBarBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(progBox), progBarBox, FALSE, FALSE, 0);
  progScaleMin = gtk_label_new("0:00");
  gtk_box_pack_start(GTK_BOX(progBarBox), progScaleMin, FALSE, FALSE, 0);
  progScaleMax = gtk_label_new(nullptr);
  gtk_box_pack_end(GTK_BOX(progBarBox), progScaleMax, FALSE, FALSE, 0);
  progScaleAdj = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
  progScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, progScaleAdj);
  gtk_box_pack_start(GTK_BOX(progBarBox), progScale, TRUE, TRUE, 0);
  g_signal_connect(progScale, "change-value",
                   G_CALLBACK(MprisModule::handleScaleChange), mprisInstance);
  g_signal_connect(progScale, "format-value",
                   G_CALLBACK(MprisModule::handleFormatValue), nullptr);

  gtk_widget_show_all(progBox);

  update();
}

void MprisModule::update() {
  if (mprisInstance->GetPlayerInfo())
    return;

  // Current Issue: Improper UTF-8 handling (Japanese characters make the markup
  // fail);
  std::string content;
  if (mprisInstance->playingTrack.title.length() > 50) {
    content = mprisInstance->playingTrack.title.substr(0, 47).append("...");
  } else {
    content = mprisInstance->playingTrack.title;
  }
  gchar *valiText = g_utf8_make_valid(content.c_str(), -1);
  gchar *finalText = g_markup_escape_text(valiText, -1);

  std::string title = "<span foreground='green'><b>";
  title += finalText;
  title += "</b></span>";

  gtk_label_set_markup(GTK_LABEL(mainLabel), title.c_str());

  // Save Resources by Not Updating if Menu is Not Visible
  if (!gtk_widget_is_visible(menuWindow))
    return;

  gtk_label_set_markup(GTK_LABEL(progTitle), title.c_str());

  // If Length is 64 Bit Int Max Value, The Track is Probably a Stream
  if (mprisInstance->playingTrack.length != 9223372036854775807) {
    gtk_label_set_label(
        GTK_LABEL(progScaleMax),
        MprisModule::timeToStr(mprisInstance->playingTrack.length).c_str());

    gtk_adjustment_set_upper(progScaleAdj, mprisInstance->playingTrack.length);
    gtk_adjustment_set_value(progScaleAdj, mprisInstance->playingTrack.currPos);
    gtk_adjustment_set_page_increment(progScaleAdj, 5);
    gtk_adjustment_set_page_size(progScaleAdj, 10);
    gtk_adjustment_set_step_increment(progScaleAdj, 5);

    gtk_widget_show(progBarBox);
  } else {
    gtk_widget_hide(progBarBox);
  }
}

void MprisModule::handlePlayPause(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PlayPause();
}

void MprisModule::chgVisibilityMenu(GtkWidget *widget, GdkEvent *e,
                                    gpointer user_data) {
  MprisModule *self = static_cast<MprisModule *>(user_data);

  if (!gtk_widget_is_visible(self->menuWindow)) {
    gtk_widget_show(self->menuWindow);
    self->update();
  } else {
    gtk_widget_hide(self->menuWindow);
  }
}

gchar *MprisModule::handleFormatValue(GtkScale *scale, gdouble value,
                                      gpointer user_data) {
  uint64_t totalSeconds = static_cast<uint64_t>(value);

  return g_strdup(timeToStr(totalSeconds).c_str());
}

gboolean MprisModule::handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                        gdouble value, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->SetPosition(static_cast<int>(value));
  return FALSE;
}

void MprisModule::handleNextTrack(GtkWidget *widget, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->NextTrack();
}
void MprisModule::handlePrevTrack(GtkWidget *widget, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PreviousTrack();
}

std::string MprisModule::timeToStr(uint64_t totalSeconds) {
  std::string timeStr;

  int hours = totalSeconds / 3600;
  totalSeconds %= 3600;
  if (hours > 0)
    timeStr = std::to_string(hours) + ":";

  int minutes = totalSeconds / 60;
  timeStr +=
      (minutes < 10 && hours > 0 ? "0" : "") + std::to_string(minutes) + ":";
  int seconds = totalSeconds % 60;
  timeStr += (seconds < 10 ? "0" : "") + std::to_string(seconds);

  return timeStr;
}
