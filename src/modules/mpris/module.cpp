#include "header/module.hpp"
#include "glibmm/ustring.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "modules/mpris/header/manager.hpp"
#include "pangomm/layout.h"
#include "sigc++/functors/mem_fun.h"
#include "utils/helper_func.hpp"

#define TAG "MprisModule"

MprisModule::MprisModule(AppContext *ctx, MprisManager *mprisMgr,
                         CommunicationBus *commBus)
    : ctx(ctx), manager(mprisMgr), commBus(commBus) {}

Gtk::Label &MprisModule::setup() {
  mainLbl.set_ellipsize(Pango::EllipsizeMode::END);

  lblAction = Gtk::GestureClick::create();
  lblAction->signal_pressed().connect(
      sigc::mem_fun(*this, &MprisModule::chgVisibilityMenu));
  mainLbl.add_controller(lblAction);

  mainLbl.set_hexpand(false);
  mainLbl.set_halign(Gtk::Align::END);

  update();
  return mainLbl;
}

void MprisModule::update() {
  if (!manager->hasPlayer())
    return;

  auto track = manager->getPlayingTrack();
  mainLbl.set_markup("<span foreground='green'><b>" + HelperFunc::ValidString(track.title) +
                     "</b></span>");
}

void MprisModule::chgVisibilityMenu(int, int, double) {
  commBus->SendMessage(
      MprisPlayPauseRequest{.correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
}
