#include "SqlCommander.h"

SqlCommander::SqlCommander() 
    : log_file_("server_output.txt")
{
    try
    {
        auto env = load_env("config.env");

        host = env["HOST"];
        dbname = env["DBNAME"];
        user = env["USER"];
        password = env["PASSWORD"];

        std::string conn_str = std::format("host={} dbname={} user={} password={}",
            host, dbname, user, password);

        conn = PQconnectdb(conn_str.c_str());

        if (PQstatus(conn) != CONNECTION_OK)
        {
            log_file_.log("SqlCommander. != CONNECTION_OK");
        }
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in SqlCommander SqlCommander(): " + std::string(e.what()));
    }
}

SqlCommander::~SqlCommander() 
{
    if (conn) 
    {
        log_file_.log("Closing connection with DB");
        PQfinish(conn);
    }
}

std::map<std::string, std::string> SqlCommander::load_env(const std::string& filename)
{
    std::map<std::string, std::string> env;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open())
    {
        log_file_.log("Failed to open config.env IN SqlCommander: " + filename);
        throw std::runtime_error("Failed to open config.env IN SqlCommander: " + filename);
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

void SqlCommander::execute_sql_command()
{
    try
    {

    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in SqlCommander ExecuteSqlCommand: " + std::string(e.what()));
    }
}

void SqlCommander::create_table()
{
    try
    {

    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in create_tablr IN SqlCommander: " + std::string(e.what()));
    }
}
