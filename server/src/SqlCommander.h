#pragma once

#include "Logger.h"

#include <libpq-fe.h>
#include <nlohmann/json.hpp>

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
    PGconn* conn_;
    Logger log_file_;

public:
    SqlCommander();
    ~SqlCommander();

public:
    std::string execute_sql_command(const std::string& type, const std::string& profile_id, const std::string& payload);

private:
    std::map<std::string, std::string> load_env(const std::string& filename);
    std::string safe_string(const char* str);


    // add new records in some table
    void add_buyers(const std::string& profile_id, const std::string& payload);
    void add_suppliers(const std::string& profile_id, const std::string& payload);
    //void add_products(std::string& request_);
    //void add_warehouses(std::string& request_);

    // get records from db
    //std::string get_buyers();
    //std::string get_suppliers();
    //std::string get_products();
    //std::string get_warehouses();

    std::string registration(const std::string& payload);
    std::string login(const std::string& payload);
    std::string get_profile(const std::string& profile_id);
    void profile_update(const std::string& profile_id, const std::string& payload);
};
