#include "SqlCommander.h"

SqlCommander::SqlCommander()
    : conn_(nullptr),
    log_file_("server_output.txt")
{
    auto env = load_env("config.env");

    host_ = env.at("HOST");
    port_ = env.at("PORT");
    dbname_ = env.at("DBNAME");
    user_ = env.at("USER");
    password_ = env.at("PASSWORD");

    std::string conn_str = fmt::format(
        "host={} port={} dbname={} user={} password={}",
        host_, port_, dbname_, user_, password_);

    conn_ = PQconnectdb(conn_str.c_str());

    if (PQstatus(conn_) != CONNECTION_OK)
    {
        log_file_.log("SqlCommander != CONNECTION_OK in SqlCommander()");
        throw std::runtime_error(std::string("DB connection failed: ") + PQerrorMessage(conn_));
    }
}

SqlCommander::~SqlCommander() 
{
    if (conn_)
    {
        log_file_.log("Closing connection with DB");
        PQfinish(conn_);
    }
}

std::map<std::string, std::string> SqlCommander::load_env(const std::string& filename)
{
    std::map<std::string, std::string> env;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open())
    {
        log_file_.log("Failed to open config.env IN SqlCommander: {}", filename);
        throw std::runtime_error("SERVER Failed to open config.env IN SqlCommander: " + filename);
    }

    while (std::getline(file, line))
    {
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);

            if (!key.empty() && !value.empty())
            {
                env[key] = value;
            }
        }
    }
    return env;
}

std::string SqlCommander::execute_sql_command(const std::string &type_, std::string &request_)
{
    try
    {
        if (type_ == "buyersAdd")
        {
            add_buyers(request_);
        }
        else if (type_ == "suppliersAdd")
        {
            add_suppliers(request_);
        }
        else if (type_ == "productsAdd")
        {
            add_products(request_);
        }
        else if (type_ == "warehousesAdd")
        {
            add_warehouses(request_);
        }
        else if (type_ == "buyersGet")
        {
            return get_buyers();
        }
        else if (type_ == "suppliersGet")
        {
            return get_suppliers();
        }
        else if (type_ == "productsGet")
        {
            return get_products();
        }
        else if (type_ == "warehousesGet")
        {
            return get_warehouses();
        }
        else
        {
            // nya
        }

        return "";
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in SqlCommander ExecuteSqlCommand: {}", e.what());

        return "";
    }
}

void SqlCommander::add_buyers(std::string& request_)
{
    try 
    {
        const char* create_table_query = R"(
            CREATE TABLE user (
                id_user INTEGER PRIMARY KEY
            );
        )";

        PGresult* res = PQexec(conn_, create_table_query);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) 
        {
            log_file_.log("Table creation failed: {}", PQerrorMessage(conn_));
            PQclear(res);
        }

        log_file_.log("Table created successfully");
        PQclear(res);

    }
    catch (const std::exception& e) 
    {
        log_file_.log("Exception in create_table in SqlCommander: {}", e.what());
    }
}
