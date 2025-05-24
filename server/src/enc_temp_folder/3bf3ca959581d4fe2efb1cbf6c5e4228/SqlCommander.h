#pragma once

#include "Logger.h"

#include <libpq-fe.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>

class SqlCommander
{
private:
    std::string host_;
    std::string port_;
    std::string dbname_;
    std::string user_;
    std::string password_;
    PGconn* conn_{ nullptr };
    Logger log_file_;

public:
    SqlCommander();
    ~SqlCommander();

public:
    std::string execute_sql_command(const std::vector<std::string>& requests_);

private:
    std::map<std::string, std::string> load_env(const std::string& filename);
    std::string create_table();
};
