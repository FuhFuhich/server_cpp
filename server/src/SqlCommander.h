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
    std::string execute_sql_command(const std::string& type_, std::string& request_);

private:
    std::map<std::string, std::string> load_env(const std::string& filename);

    // add new records in some table
    void add_buyers(std::string& request_);
    void add_suppliers(std::string& request_);
    void add_products(std::string& request_);
    void add_warehouses(std::string& request_);

    // get records from db
    std::string get_buyers();
    std::string get_suppliers();
    std::string get_products();
    std::string get_warehouses();
};
