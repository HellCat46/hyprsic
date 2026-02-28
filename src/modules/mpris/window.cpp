#include "window.hpp"
#include "../../utils/helper_func.hpp"
#include "gtk-layer-shell.h"

MprisWindow::MprisWindow(AppContext *ctx, MprisManager *mprisMgr)
    : ctx(ctx), manager(mprisMgr) {}

void MprisWindow::init() {

  menuWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_layer_init_for_window(GTK_WINDOW(menuWin));
  gtk_layer_set_layer(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_LAYER_OVERLAY);
  gtk_layer_set_namespace(GTK_WINDOW(menuWin), "popup");
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
  gtk_layer_set_anchor(GTK_WINDOW(menuWin), GTK_LAYER_SHELL_EDGE_LEFT, FALSE);
  gtk_widget_set_size_request(menuWin, 500, -1);

  GtkWidget *progBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(menuWin), progBox);
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
                   G_CALLBACK(MprisWindow::handlePrevTrack), this);

  GtkWidget *progEventListener = gtk_event_box_new();
  progTtl = gtk_label_new(nullptr);
  gtk_widget_set_halign(progTtl, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand(progTtl, TRUE);
  gtk_container_add(GTK_CONTAINER(progEventListener), progTtl);
  gtk_box_pack_start(GTK_BOX(titleBox), progEventListener, FALSE, FALSE, 0);
  g_signal_connect(progEventListener, "button-press-event",
                   G_CALLBACK(MprisWindow::handlePlayPause), manager);

  // Next Button
  GtkWidget *titleNext = gtk_button_new_with_label(">");
  gtk_box_pack_end(GTK_BOX(titleBox), titleNext, FALSE, FALSE, 0);
  g_signal_connect(titleNext, "clicked",
                   G_CALLBACK(MprisWindow::handleNextTrack), this);

  // Scale for Track Progress
  progBarBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(progBox), progBarBox, FALSE, FALSE, 0);
  scaleMin = gtk_label_new("0:00");
  gtk_box_pack_start(GTK_BOX(progBarBox), scaleMin, FALSE, FALSE, 0);
  scaleMax = gtk_label_new(nullptr);
  gtk_box_pack_end(GTK_BOX(progBarBox), scaleMax, FALSE, FALSE, 0);
  scaleAdj = gtk_adjustment_new(0, 0, 0, 0, 0, 0);
  scale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, scaleAdj);
  gtk_box_pack_start(GTK_BOX(progBarBox), scale, TRUE, TRUE, 0);
  g_signal_connect(scale, "change-value",
                   G_CALLBACK(MprisWindow::handleScaleChange), manager);
  g_signal_connect(scale, "format-value",
                   G_CALLBACK(MprisWindow::handleFormatValue), nullptr);

  gtk_widget_show_all(progBox);
}

void MprisWindow::update() {
  // Save Resources by Not Updating if Menu is Not Visible
  if (!(manager->hasPlayer && gtk_widget_is_visible(menuWin)))
    return;

  gchar *finalText = HelperFunc::ValidString(manager->playingTrack.title);
  std::string title = "<span foreground='green'><b>";
  title += finalText;
  title += "</b></span>";

  gtk_label_set_markup(GTK_LABEL(progTtl), title.c_str());
  manager->GetPosition();

  // If Length is 64 Bit Int Max Value, The Track is Probably a Stream
  if (manager->playingTrack.length != 9223372036854775807) {
    gtk_label_set_label(
        GTK_LABEL(scaleMax),
        MprisWindow::timeToStr(manager->playingTrack.length).c_str());

    gtk_adjustment_set_upper(scaleAdj, manager->playingTrack.length);
    gtk_adjustment_set_value(scaleAdj, manager->playingTrack.currPos);
    gtk_adjustment_set_page_increment(scaleAdj, 5);
    gtk_adjustment_set_page_size(scaleAdj, 10);
    gtk_adjustment_set_step_increment(scaleAdj, 5);

    gtk_widget_show(progBarBox);
  } else {
    gtk_widget_hide(progBarBox);
  }
}

void MprisWindow::handlePlayPause(GtkWidget *widget, GdkEvent *e,
                                  gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PlayPause();
}

gchar *MprisWindow::handleFormatValue(GtkScale *scale, gdouble value,
                                      gpointer user_data) {
  uint64_t totalSeconds = static_cast<uint64_t>(value);

  return g_strdup(timeToStr(totalSeconds).c_str());
}

gboolean MprisWindow::handleScaleChange(GtkRange *range, GtkScrollType *scroll,
                                        gdouble value, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->SetPosition(static_cast<int>(value));
  return FALSE;
}

void MprisWindow::handleNextTrack(GtkWidget *widget, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->NextTrack();
}
void MprisWindow::handlePrevTrack(GtkWidget *widget, gpointer user_data) {
  MprisManager *mprisInstance = static_cast<MprisManager *>(user_data);
  mprisInstance->PreviousTrack();
}

std::string MprisWindow::timeToStr(uint64_t totalSeconds) {
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

bool MprisWindow::isVisible() const { return gtk_widget_is_visible(menuWin); }

void MprisWindow::chgVisibility(bool visible) {
  if (visible) {
    update();
    gtk_widget_show(menuWin);
  } else
    gtk_widget_hide(menuWin);
}
