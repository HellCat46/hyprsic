#pragma once
#include "SQLiteCpp/Database.h"


class DBManager {  
    SQLite::Database* localDB;

    public:
        DBManager();
};