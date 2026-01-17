#pragma once
#include "../logging/manager.hpp"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include <memory>

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
  std::map<std::string, NotificationRecord> notificationCache;
  DBManager(LoggingManager *logMgr);
  int insertNotification(const NotificationRecord *notif);
  int removeNotification(const std::string &id);
};
