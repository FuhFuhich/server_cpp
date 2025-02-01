#pragma once

#include <libpq-fe.h>
#include <string>
#include <map>
#include <fstream>

class SqlCommander
{
private:
    std::string host;
    std::string dbname;
    std::string user;
    std::string password;
    PGconn* conn;

public:
    SqlCommander();
    ~SqlCommander();
    std::map<std::string, std::string> load_env(const std::string& filename);
};
