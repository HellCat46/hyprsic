#pragma once
#include "gtkmm/label.h"
#include "manager.hpp"

class BluetoothModule {
  AppContext *ctx;
  BluetoothManager *manager;

  Gtk::Label mainLbl;

public:
  BluetoothModule(AppContext *ctx, BluetoothManager *manager);
  // UI Prep Functions
  Gtk::Label &setup();
};
