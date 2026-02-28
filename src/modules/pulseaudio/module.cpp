#include "module.hpp"
#include "gdk-pixbuf/gdk-pixbuf.h"
#include "manager.hpp"
#include <vector>

#define TAG "PulseAudioModule"

PulseAudioModule::PulseAudioModule(PulseAudioManager *paMgr, AppContext *ctx, PulseAudioWindow *window)
    : manager(paMgr), window(window), ctx(ctx), setupComp(false) {}

std::vector<GtkWidget *> PulseAudioModule::setup() {
  std::vector<GtkWidget *> widgets;

  inEvtBox = gtk_event_box_new();
  barInIcon = gtk_image_new_from_pixbuf(nullptr);
  gtk_container_add(GTK_CONTAINER(inEvtBox), barInIcon);
  if(GDK_IS_PIXBUF(window->inUnmuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(barInIcon), window->inUnmuteIcon);
  }

  widgets.push_back(inEvtBox);

  outEvtBox = gtk_event_box_new();
  barOutIcon = gtk_image_new_from_pixbuf(nullptr);
  gtk_container_add(GTK_CONTAINER(outEvtBox), barOutIcon);
  if(GDK_IS_PIXBUF(window->outUnmuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(barOutIcon), window->outUnmuteIcon);
  }
  
  widgets.push_back(outEvtBox);

  gtk_widget_show_all(inEvtBox);
  gtk_widget_show_all(outEvtBox);

  g_signal_connect(inEvtBox, "button-press-event", G_CALLBACK(handleIconClick),
                   this);
  g_signal_connect(outEvtBox, "button-press-event", G_CALLBACK(handleIconClick),
                   this);

  setupComp = true;
  return widgets;
}

void PulseAudioModule::update() {
  if (!setupComp)
    return;

  auto it = manager->outDevs.find(manager->defOutput);
  if (it != manager->outDevs.end()) {
    updateControls(it->second.mute, barOutIcon);
  }

  it = manager->inDevs.find(manager->defInput);
  if (it != manager->inDevs.end()) {
    updateControls(it->second.mute, barInIcon);
  }
}

void PulseAudioModule::updateControls(bool mute, GtkWidget *icon) {
  if (icon == barOutIcon && GDK_IS_PIXBUF(window->outUnmuteIcon) && GDK_IS_PIXBUF(window->outMuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon), mute ? window->outUnmuteIcon
                                                    : window->outMuteIcon);
  } else if (icon == barInIcon && GDK_IS_PIXBUF(window->inUnmuteIcon) &&
             GDK_IS_PIXBUF(window->inMuteIcon)) {
    gtk_image_set_from_pixbuf(GTK_IMAGE(icon),
                              mute ? window->inUnmuteIcon : window->inMuteIcon);
  }
}

void PulseAudioModule::handleIconClick(GtkWidget *widget,
                                       GdkEventButton *evtBtn, gpointer data) {
  PulseAudioModule *self = static_cast<PulseAudioModule *>(data);
  if (evtBtn->button == 3) {
    self->ctx->showCtrlWindow("pulseaudio", 400, -1);
    return;
  }

  if (widget == self->outEvtBox) {
    self->window->toggleMute(widget, data, true);
  } else if (widget == self->inEvtBox) {
    self->window->toggleMute(widget, data, false);
  }
}

