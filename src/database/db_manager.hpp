#pragma once
#include "SQLiteCpp/Database.h"
#include "SQLiteCpp/Statement.h"
#include "../logging/manager.hpp"
#include <vector>

struct NotificationRecord {
    std::string id;
    std::string app_name;
    std::string summary;
    std::string body;
    std::string timestamp;
};

class DBManager {  
    LoggingManager* logger;
    SQLite::Database* localDB;
    SQLite::Statement* insertStmt;
    SQLite::Statement* fetchStmt;
    SQLite::Statement* deleteStmt;

    public:
        DBManager(LoggingManager* logMgr);
        int insertNotification(const NotificationRecord* notif);
        std::vector<NotificationRecord> fetchNotifications(int limit, int offset);
        int removeNotification(const std::string& id);
};