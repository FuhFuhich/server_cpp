#pragma once

#include <libpq-fe.h>
#include <string>
#include <map>
#include <fstream>
#include <format>

class SqlCommander
{
private:
    std::string host;
    std::string dbname;
    std::string user;
    std::string password;
    PGconn* conn;
    std::ofstream log_file_;

public:
    SqlCommander();
    ~SqlCommander();
    std::map<std::string, std::string> load_env(const std::string& filename);
    void ExecuteSqlCommand();
};
