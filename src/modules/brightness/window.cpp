#include "header/window.hpp"
#include "gtkmm/adjustment.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/scale.h"
#include "modules/brightness/header/manager.hpp"
#include "services/header/comm_types.hpp"

#define TAG "BrightnessWindow"

BrightnessWindow::BrightnessWindow(AppContext *ctx, CommunicationBus* commbus, BrightnessManager* manager) : ctx(ctx), manager(manager), commBus(commbus){}

void BrightnessWindow::init() {
    winBox.set_orientation(Gtk::Orientation::VERTICAL);
    winBox.set_spacing(5);
  
    Gtk::Label titleLbl;
    titleLbl.set_markup("<big><b>Brightness</b></big>");
    winBox.append(titleLbl);

    adjWid = Gtk::Adjustment::create(0, 0, 100, 5, 10, 0);
  
    
    scale.set_value_pos(Gtk::PositionType::TOP);
    scale.set_adjustment(adjWid);
    scale.signal_change_value().connect(
        [this](Gtk::ScrollType, double value) -> bool {
            this->handleScaleChange(value);
            return false;
        },
        false);
    
    winBox.append(scale);
    winBox.append(lbl);
    winBox.set_margin(20);
    
    ctx->addModule(winBox, "brightness");
    update();
}

void BrightnessWindow::update() {
    short brightness = manager->getLvl();
    
    if (brightness >= 0 && brightness != adjWid->get_value()) {
        adjWid->set_value(brightness);
        lbl.set_text(std::to_string(brightness) + "%");
    }
}

void BrightnessWindow::handleScaleChange(double value) {
    if(manager->getLvl() == short(value)) return;
    
    commBus->SendMessage(BrtSetLevelRequest{short(value), ModuleType::BRIGHTNESS, commBus->GetNewCorId()}, Priority::IMMEDIATE);
    update();
}
