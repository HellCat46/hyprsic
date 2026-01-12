#include "module.hpp"
#include "glib-object.h"
#include "glib.h"
#include "glibconfig.h"
#include "gtk/gtk.h"
#include <cerrno>
#include <string>

#define TAG "MprisModule"

MprisModule::MprisModule(AppContext *ctx) : mprisInstance(ctx) {
  logger = &ctx->logging;
}

void MprisModule::setup(GtkWidget *mainBox) {
  GtkWidget *middleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_box_pack_end(GTK_BOX(mainBox), middleBox, FALSE, FALSE, 0);

  mainLabel = gtk_label_new(nullptr);
  GtkWidget *labelAction = gtk_event_box_new();
  g_signal_connect(labelAction, "button-press-event",
                   G_CALLBACK(MprisModule::showPopOverMenu), this);

  gtk_container_add(GTK_CONTAINER(labelAction), mainLabel);
  gtk_box_pack_start(GTK_BOX(middleBox), labelAction, FALSE, FALSE, 0);

  // Popover Menu
  popOverMenu = gtk_popover_new(labelAction);
  gtk_popover_set_modal(GTK_POPOVER(popOverMenu), FALSE);
  gtk_popover_set_position(GTK_POPOVER(popOverMenu), GTK_POS_TOP);
  gtk_widget_set_margin_bottom(popOverMenu, 30);

  GtkWidget *progBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(popOverMenu), progBox);
  gtk_widget_set_margin_bottom(progBox, 10);
  gtk_widget_set_margin_top(progBox, 10);
  gtk_widget_set_margin_start(progBox, 10);
  gtk_widget_set_margin_end(progBox, 10);

  GtkWidget *topBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(progBox), topBox, FALSE, FALSE, 0);
  gtk_widget_set_margin_bottom(topBox, 20);

  GtkWidget *winTitle = gtk_label_new(nullptr);
  gtk_box_pack_start(GTK_BOX(topBox), winTitle, FALSE, FALSE, 0);
  gtk_label_set_markup(GTK_LABEL(winTitle),
                       "<b>Now Playing - Player Control</b>");
  gtk_label_set_xalign(GTK_LABEL(winTitle), 0);

  GtkWidget *closeButton = gtk_button_new_with_label("✖");
  gtk_box_pack_end(GTK_BOX(topBox), closeButton, FALSE, FALSE, 0);
  g_signal_connect(closeButton, "clicked",
                   G_CALLBACK(MprisModule::hidePopOverMenu), this);

  GtkWidget *titleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(progBox), titleBox, FALSE, FALSE, 0);

  // Prev Button
  GtkWidget *titlePrev = gtk_button_new_with_label("<");
  gtk_box_pack_start(GTK_BOX(titleBox), titlePrev, FALSE, FALSE, 0);
  g_signal_connect(titlePrev, "clicked", G_CALLBACK(MprisModule::handlePrevTrack), this);

  GtkWidget *progEventListener = gtk_event_box_new();
  progTitle = gtk_label_new(nullptr);
  gtk_container_add(GTK_CONTAINER(progEventListener), progTitle);
  gtk_box_pack_start(GTK_BOX(titleBox), progEventListener, FALSE, FALSE, 0);
  g_signal_connect(progEventListener, "button-press-event",
                   G_CALLBACK(MprisModule::handlePlayPause), &mprisInstance);

  // Next Button
  GtkWidget *titleNext = gtk_button_new_with_label(">");
  gtk_box_pack_end(GTK_BOX(titleBox), titleNext, FALSE, FALSE, 0);
  g_signal_connect(titleNext, "clicked", G_CALLBACK(MprisModule::handleNextTrack), this);

  progScaleAdj = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
  progScale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, progScaleAdj);
  gtk_box_pack_start(GTK_BOX(progBox), progScale, FALSE, FALSE, 0);
  g_signal_connect(progScale, "change-value",
                   G_CALLBACK(MprisModule::handleScaleChange), &mprisInstance);
  g_signal_connect(progScale, "format-value",
                   G_CALLBACK(MprisModule::handleFormatValue), nullptr);

  gtk_widget_show_all(popOverMenu);

  Update();
}

void MprisModule::Update() {
  if (mprisInstance.GetPlayerInfo())
    return;

  // Current Issue: Improper UTF-8 handling (Japanese characters make the markup
  // fail);
  std::string title = "<span foreground='green'><b>";
  if (mprisInstance.playingTrack.title.length() > 50) {
    title += mprisInstance.playingTrack.title.substr(0, 47).append("...");
  } else {
    title += mprisInstance.playingTrack.title;
  }
  title += "</b></span>";

  gtk_label_set_markup(GTK_LABEL(progTitle), title.c_str());
  gtk_label_set_markup(GTK_LABEL(mainLabel), title.c_str());

  gtk_adjustment_set_upper(progScaleAdj, mprisInstance.playingTrack.length);
  gtk_adjustment_set_value(progScaleAdj, mprisInstance.playingTrack.currPos);
  gtk_adjustment_set_page_increment(progScaleAdj, 5);
  gtk_adjustment_set_page_size(progScaleAdj, 10);
  gtk_adjustment_set_step_increment(progScaleAdj, 5);
}

void MprisModule::handlePlayPause(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PlayPause();
}

void MprisModule::showPopOverMenu(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data) {
  MprisModule *self = static_cast<MprisModule *>(user_data);

  self->Update();

  gtk_popover_popup(GTK_POPOVER(self->popOverMenu));
}

void MprisModule::hidePopOverMenu(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data) {
  MprisModule *self = static_cast<MprisModule *>(user_data);
  gtk_popover_popdown(GTK_POPOVER(self->popOverMenu));
}

gchar *MprisModule::handleFormatValue(GtkScale *scale, gdouble value,
                                      gpointer user_data) {
  int totalSeconds = static_cast<int>(value);
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

  return g_strdup(timeStr.c_str());
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
