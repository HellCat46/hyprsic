#pragma once
#include "../logging/manager.hpp"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include <list>
#include <memory>
#include <string>

struct NotificationRecord {
  std::string id;
  std::string app_name;
  std::string summary;
  std::string body;
  std::string timestamp;
};

class DBManager {
  LoggingManager* logger;
  std::unique_ptr<SQLite::Database> localDB;
  std::unique_ptr<SQLite::Statement> insertStmt;
  std::unique_ptr<SQLite::Statement> deleteStmt;

public:
  std::list<NotificationRecord> notifList;
  DBManager(LoggingManager *logMgr);
  bool insertNotification(const NotificationRecord *notif);
  bool removeNotification(const std::string &id, std::list<NotificationRecord>::iterator& it);
  bool clearAllNotifications();
  
  std::string getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return std::string(buf);
  }
};
