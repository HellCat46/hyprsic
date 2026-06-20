#include "header/window.hpp"
#include "glibmm/ustring.h"
#include "gtkmm/adjustment.h"
#include "gtkmm/button.h"
#include "gtkmm/enums.h"
#include "gtkmm/gestureclick.h"
#include "gtkmm/label.h"
#include "modules/mpris/header/manager.hpp"
#include "services/header/comm_bus.hpp"
#include "sigc++/functors/mem_fun.h"
#include "utils/helper_func.hpp"
#include <climits>
#include <string>

#define TAG "MprisWindow"

MprisWindow::MprisWindow(AppContext *ctx, CommunicationBus *commBus,
                         MprisManager *mprisMgr)
    : ctx(ctx), manager(mprisMgr), commBus(commBus) {}

void MprisWindow::init() {
  progBox.set_orientation(Gtk::Orientation::VERTICAL);
  progBox.set_spacing(5);
  progBox.set_margin(10);

  Gtk::Label winTitle;
  winTitle.set_markup("<b>Now Playing - Player Control</b>");
  winTitle.set_margin_bottom(20);
  progBox.append(winTitle);

  Gtk::Box titleBox;
  titleBox.set_spacing(5);
  progBox.append(titleBox);

  // Prev Button
  Gtk::Button titlePrev{"<"};
  titlePrev.signal_clicked().connect(
      sigc::mem_fun(*this, &MprisWindow::handlePrevTrack));
  titleBox.append(titlePrev);

  // Track Title & Play/Pause Button
  auto progEvtListener = Gtk::GestureClick::create();
  progEvtListener->signal_pressed().connect(
      sigc::mem_fun(*this, &MprisWindow::handlePlayPause));

  progTtl.set_halign(Gtk::Align::CENTER);
  progTtl.set_hexpand(true);
  progTtl.add_controller(progEvtListener);
  titleBox.append(progTtl);

  // Next Button
  Gtk::Button titleNext{">"};
  titleNext.signal_clicked().connect(
      sigc::mem_fun(*this, &MprisWindow::handleNextTrack));
  titleBox.append(titleNext);

  // Scale for Track Progress
  progBarBox.set_spacing(5);
  progBox.append(progBarBox);

  scaleMin.set_text("0:00");
  progBarBox.append(scaleMin);
  progBarBox.append(scaleMax);

  scaleAdj = Gtk::Adjustment::create(0, 0, 0, 5, 5, 10);
  scale.set_adjustment(scaleAdj);
  progBarBox.append(scale);
  scale.signal_change_value().connect(
      [this](Gtk::ScrollType, double value) {
        handleScaleChange(value);
        return false;
      },
      false);
}

void MprisWindow::update() {
  // Save Resources by Not Updating if Menu is Not Visible
  if (!(manager->hasPlayer() && progBox.is_visible()))
    return;

  auto track = manager->getPlayingTrack();
  progTtl.set_markup("<span foreground='green'><b>" + HelperFunc::ValidString(track.title) +
                     "</b></span>");

  // TODO(hyprsic): Send message to request position update when response is
  // ready commBus->SendMessage(MprisGetPositionRequest{.moduleType =
  // ModuleType::MPRIS }, Priority::HIGH); manager->GetPosition();

  // If Length is 64 Bit Int Max Value, The Track is Probably a Stream
  if (track.length != ULONG_MAX) {
    scaleMax.set_label(MprisWindow::timeToStr(track.length));

    scaleAdj->set_upper(track.length);
    scaleAdj->set_value(track.currPos);

    progBarBox.show();
  } else {
    progBarBox.hide();
  }
}

void MprisWindow::handlePlayPause(int, int, double) {

  commBus->SendMessage(
      MprisPlayPauseRequest{.correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
}

void MprisWindow::handleScaleChange(double value) {

  commBus->SendMessage(
      MprisSetPositionRequest{.position = static_cast<uint64_t>(value),
                              .correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
}

void MprisWindow::handleNextTrack() {
  commBus->SendMessage(
      MprisNextTrackRequest{.correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
}

void MprisWindow::handlePrevTrack() {
  commBus->SendMessage(
      MprisPreviousTrackRequest{.correlationId = commBus->GetNewCorId()},
      Priority::HIGH);
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
