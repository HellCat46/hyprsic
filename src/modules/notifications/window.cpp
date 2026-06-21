#include "header/window.hpp"
#include "gtkmm/box.h"
#include "gtkmm/button.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/switch.h"
#include "gtkmm/scrolledwindow.h"
#include "sigc++/functors/mem_fun.h"
#include "utils/helper_func.hpp"

#define TAG "NotificationWindow"

NotificationWindow::NotificationWindow(AppContext *ctx,
                                       CommunicationBus *commBus,
                                       NotificationManager *manager)
    : ctx(ctx), manager(manager), commBus(commBus) {}

void NotificationWindow::init() {
  menuBox.set_orientation(Gtk::Orientation::VERTICAL);
  menuBox.set_spacing(10);
  menuBox.set_margin(10);

  // TopBar Box

  Gtk::Box topBar{Gtk::Orientation::HORIZONTAL, 5};
  menuBox.append(topBar);

  Gtk::Label notifTitle;
  notifTitle.set_markup("<big><b>Notifications</b></big>");
  topBar.append(notifTitle);

  Gtk::Button clearBtn{"Clear All"};
  clearBtn.signal_clicked().connect(
      sigc::mem_fun(*this, &NotificationWindow::handleClearAll));
  topBar.append(clearBtn);

  // Do Not Disturb Toggle
  Gtk::Box dndBox{Gtk::Orientation::HORIZONTAL, 5};
  dndBox.set_margin_bottom(10);
  dndBox.set_margin_start(10);
  dndBox.set_margin_end(10);

  Gtk::Label dndLbl;
  dndLbl.set_markup("<b>Do Not Disturb</b>");
  dndLbl.set_halign(Gtk::Align::START);
  dndBox.append(dndLbl);

  Gtk::Switch dndSwitch;
  dndSwitch.signal_state_set().connect(
      [this](bool state) -> bool {
        handleDndToggle(state);
        return false;
      },
      false);
  dndBox.append(dndSwitch);
  menuBox.append(dndBox);

  // Scrollable Window for Notifications
  Gtk::ScrolledWindow scrollWin;
  scrollWin.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::AUTOMATIC);
  scrollWin.set_size_request(400, 300);
  menuBox.append(scrollWin);

  scrollWin.set_child(scrollWinBox);

  ctx->addModule(menuBox, "notifications");
  update(true);
}

void NotificationWindow::update(bool force) {
  if (!menuBox.is_visible() && !force)
    return;

  for (auto notif = ctx->dbManager.notifList.begin();
       notif != ctx->dbManager.notifList.end(); ++notif) {
    // If we've already processed this notification, skip the whole list as they
    // are ordered by timestamp
    if (notifLookup.find(notif->id) != notifLookup.end())
      break;


    Gtk::Box contentBox{Gtk::Orientation::VERTICAL, 5};

    Gtk::Box topContent{Gtk::Orientation::HORIZONTAL, 5};
    contentBox.append(topContent);

    // ctx->logger.LogInfo(TAG, notif->app_name + " - " + notif->summary + " - " + notif->body + " - " + notif->timestamp + " - " + notif->id);

    Gtk::Label appName;
    appName.set_markup("<b>" + HelperFunc::ValidString(notif->app_name) + "</b> - ");
    appName.set_halign(Gtk::Align::START);
    topContent.append(appName);

    Gtk::Label timestampLbl;
    timestampLbl.set_markup("<i>" + HelperFunc::ValidString(notif->timestamp) + "</i>");
    timestampLbl.set_halign(Gtk::Align::END);
    topContent.append(timestampLbl);

    Gtk::Label titleLbl;
    titleLbl.set_markup(
        (std::string("<b>Summary:</b> ") +
         (notif->summary.size() > 25
              ? HelperFunc::ValidString(notif->summary.substr(0, 22) + "...")
              : HelperFunc::ValidString(notif->summary))));

    titleLbl.set_wrap(true);
    titleLbl.set_halign(Gtk::Align::START);
    contentBox.append(titleLbl);

    Gtk::Label bodyLbl;
    bodyLbl.set_markup(HelperFunc::ValidString(notif->body));
    bodyLbl.set_wrap(true);
    bodyLbl.set_halign(Gtk::Align::START);
    contentBox.append(bodyLbl);

    Gtk::Button removeBtn{"✖"};
    removeBtn.signal_clicked().connect([this, notifId = notif->id]() {
        deleteNotificationCb(notifId);
        
    });

    auto& notifBox = notifLookup.emplace(notif->id, NotifListItem{notif}).first->second.widget;
    notifBox.set_margin(5);
    notifBox.append(contentBox);
    notifBox.append(removeBtn);
    scrollWinBox.append(notifBox);

    ;
  }
}

void NotificationWindow::deleteNotificationCb(std::string notifId) {

  auto it = notifLookup.find(notifId);
  if (it != notifLookup.end()) {

    ctx->dbManager.removeNotification(notifId, it->second.it);
    it->second.widget.unparent();
    notifLookup.erase(it);
  }
}

void NotificationWindow::handleDndToggle(bool state) {
  manager->dnd = state;

  std::string msg =
      "Do Not Disturb Mode " + std::string(state ? "Enabled" : "Disabled");
  ctx->logger.LogInfo(TAG, msg);
  ctx->showUpdateWindow(state ? "dnd_on" : "dnd_off", msg);
}

void NotificationWindow::handleClearAll() {
  ctx->dbManager.clearAllNotifications();

  for (auto &pair : notifLookup) {
    pair.second.widget.unparent();
  }
  notifLookup.clear();
}
