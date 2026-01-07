#include "db_manager.hpp"
#include "glib.h"
#include <string>

DBManager::DBManager() : localDB(nullptr) {
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

  localDB = new SQLite::Database(dbPath,
                                 SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
}
