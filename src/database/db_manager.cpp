#include "db_manager.hpp"
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "glib.h"
#include <memory>
#include <string>

#define TAG "DBManager"

DBManager::DBManager(LoggingManager *logMgr) : localDB(nullptr) {
  logger = logMgr;
  const char *xdgPath = std::getenv("XDG_DATA_HOME");

  std::string dbPath;
  if (xdgPath) {
    dbPath = std::string(xdgPath) + "/hyprsic/";
  } else {
    const char *homePath = std::getenv("HOME");
    if (homePath) {
      dbPath = std::string(homePath) + "/.local/share/hyprsic/";
    }
  }

  if (dbPath.size() != 0) {
    g_mkdir_with_parents(dbPath.c_str(), 0755);
  }

  dbPath += "hyprsic.db3";
  localDB = std::unique_ptr<SQLite::Database>(new SQLite::Database(
      dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE));

  // Create Notifications Table
  std::string tableCreationQuery = R"(
      CREATE TABLE IF NOT EXISTS notifications (
        id TEXT PRIMARY KEY,
        app_name TEXT,
        summary TEXT,
        body TEXT,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
      );
  )";
  localDB->exec(tableCreationQuery);

  // Create Indexes
  std::string indexCreationQuery = R"(
      CREATE INDEX IF NOT EXISTS idx_notif_timestamp ON notifications(timestamp DESC);
  )";
  localDB->exec(indexCreationQuery);

  // Create Triggers
  std::string triggerCreationQuery = R"(
      CREATE TRIGGER IF NOT EXISTS delete_old_notifications
      AFTER INSERT ON notifications
      BEGIN
        DELETE FROM notifications
        WHERE id NOT IN (
          SELECT id FROM notifications
          ORDER BY timestamp DESC
          LIMIT 100
        );
      END;

      CREATE TRIGGER IF NOT EXISTS cleanup_old_notifications
      AFTER INSERT ON notifications
      BEGIN
          DELETE FROM notifications
          WHERE timestamp < strftime('%s', 'now', '-30 days');
      END;
  )";
  localDB->exec(triggerCreationQuery);

  // Prepare SQL Statements
  insertStmt = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(
      *localDB, "INSERT INTO notifications (id, app_name, summary, body) "
                "VALUES (?, ?, ?, ?);"));
  deleteStmt = std::unique_ptr<SQLite::Statement>(new SQLite::Statement(
      *localDB, "DELETE FROM notifications WHERE id = ?;"));

  SQLite::Statement countStmt(*localDB,
                              "SELECT id, app_name, summary, body, timestamp "
                              "FROM notifications ORDER BY timestamp DESC;");
  while (countStmt.executeStep()) {
    NotificationRecord notif;
    notif.id = countStmt.getColumn(0).getString();
    notif.app_name = countStmt.getColumn(1).getString();
    notif.summary = countStmt.getColumn(2).getString();
    notif.body = countStmt.getColumn(3).getString();
    notif.timestamp = countStmt.getColumn(4).getString();

    notificationCache[notif.id] = notif;
  }

  // logger->LogDebug(TAG, "Notifications Loaded: " +
  //                           std::to_string(notificationCache.size()));
}

int DBManager::insertNotification(const NotificationRecord *notif) {
  try {
    insertStmt->bind(1, notif->id);
    insertStmt->bind(2, notif->app_name);
    insertStmt->bind(3, notif->summary);
    insertStmt->bind(4, notif->body);

    insertStmt->exec();
    insertStmt->reset();

    notificationCache[notif->id] = *notif;
    return 0;
  } catch (const std::exception &e) {
    logger->LogError(TAG, "Failed to insert notification ID: " + notif->id +
                              " Error: " + e.what());
    insertStmt->reset();

    return -1;
  }
}

int DBManager::removeNotification(const std::string &id) {
  try {
    deleteStmt->bind(1, id);
    deleteStmt->exec();
    deleteStmt->reset();

    notificationCache.erase(id);
    return 0;
  } catch (const std::exception &e) {
    logger->LogError(TAG, "Failed to delete notification ID: " + id +
                              " Error: " + e.what());
    deleteStmt->reset();

    return -1;
  }
}
