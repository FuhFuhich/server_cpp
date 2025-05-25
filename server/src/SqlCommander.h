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
    std::string execute_sql_command(const std::string& type, const std::string& chat_id, const std::string& payload);

private:
    std::map<std::string, std::string> load_env(const std::string& filename);

    // add new records in some table
    void add_buyers(const std::string& chat_id, const std::string& payload);
    //void add_suppliers(std::string& request_);
    //void add_products(std::string& request_);
    //void add_warehouses(std::string& request_);

    // get records from db
    //std::string get_buyers();
    //std::string get_suppliers();
    //std::string get_products();
    //std::string get_warehouses();
};
