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
    void add_products(const std::string& profile_id, const std::string& payload);
    void add_warehouses(const std::string& profile_id, const std::string& payload);

    // get records from db
    std::string get_buyers(const std::string& profile_id);
    std::string get_suppliers(const std::string& profile_id);
    std::string get_products(const std::string& profile_id);
    std::string get_warehouses(const std::string& profile_id);

    // delete records
    void delete_buyers(const std::string& profile_id, const std::string& payload);
    void delete_suppliers(const std::string& profile_id, const std::string& payload);
    void delete_products(const std::string& profile_id, const std::string& payload);
    void delete_warehouses(const std::string& profile_id, const std::string& payload);

    // update records
    void update_buyers(const std::string& profile_id, const std::string& payload);
    void update_suppliers(const std::string& profile_id, const std::string& payload);
    void update_products(const std::string& profile_id, const std::string& payload);
    void update_warehouses(const std::string& profile_id, const std::string& payload);

    std::string registration(const std::string& payload);
    std::string login(const std::string& payload);
    std::string get_profile(const std::string& profile_id);
    void profile_update(const std::string& profile_id, const std::string& payload);
};
