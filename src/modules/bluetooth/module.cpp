#include "header/module.hpp"
#include "gtkmm/label.h"

#define TAG "BluetoothModule"

BluetoothModule::BluetoothModule(AppContext *ctx, BluetoothManager *manager)
    : ctx(ctx), manager(manager) {}

Gtk::Label& BluetoothModule::setup() {
  mainLbl.set_text("");
  mainLbl.set_margin_start(10);
 
  return mainLbl;
}
