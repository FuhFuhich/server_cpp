#include "SqlCommander.h"

SqlCommander::SqlCommander() 
{
    auto env = load_env("config.env");

    host = env["HOST"];
    dbname = env["DBNAME"];
    user = env["USER"];
    password = env["PASSWORD"];

    std::string conn_str = "host=" + host + " dbname=" + dbname +
        " user=" + user + " password=" + password;

    conn = PQconnectdb(conn_str.c_str());

    if (PQstatus(conn) != CONNECTION_OK) 
    {
        // Добавить вывод логов
    }
}

SqlCommander::~SqlCommander() 
{
    if (conn) 
    {
        PQfinish(conn);
    }
}

std::map<std::string, std::string> SqlCommander::load_env(const std::string& filename)
{
    std::map<std::string, std::string> env;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line))
    {
        size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos)
        {
            std::string key = line.substr(0, eq_pos);
            std::string value = line.substr(eq_pos + 1);
            env[key] = value;
        }
    }
    return env;
}
