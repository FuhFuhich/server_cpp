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

        std::string conn_str = fmt::format("host={} dbname={} user={} password={}",
            host, dbname, user, password);

        conn = PQconnectdb(conn_str.c_str());

        if (PQstatus(conn) != CONNECTION_OK)
        {
            log_file_.log("SqlCommander != CONNECTION_OK in SqlCommander()");
        }
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in SqlCommander SqlCommander(): {}", e.what());
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

std::map<std::string, std::string> SqlCommander::load_env(std::string&& filename)
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

/*
void SqlCommander::execute_sql_command()
{
    try
    {

    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in SqlCommander ExecuteSqlCommand: {}", e.what());
    }
}
*/

void SqlCommander::create_table()
{
    try
    {
        const char* create_table_query = R"(
            CREATE TABLE user(id_user INTEGER PRIMARY KEY);)";

        PGresult* res = PQexec(conn, create_table_query);

        if (PQresultStatus(res) != PGRES_COMMAND_OK) 
        {
            log_file_.log("Table creation failed: {}", PQerrorMessage(conn));
        }
        else 
        {
            log_file_.log("Table created successfully");
        }

        PQclear(res);
    }
    catch (const std::exception& e)
    {
        log_file_.log("Exception in create_tablr IN SqlCommander: {}", e.what());
    }
}
