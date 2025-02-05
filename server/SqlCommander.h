#pragma once

#include "Logger.h"

#include <libpq-fe.h>

#include <iostream>
#include <string>
#include <map>

class SqlCommander
{
private:
    std::string host;
    std::string dbname;
    std::string user;
    std::string password;
    PGconn* conn;
    Logger log_file_;

public:
    SqlCommander();
    ~SqlCommander();
    std::map<std::string, std::string> load_env(std::string&& filename);
    void execute_sql_command();
    void create_table();
};
