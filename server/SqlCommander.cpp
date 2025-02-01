#include "SqlCommander.h"

SqlCommander::SqlCommander() 
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

        log_file_.open("server_output.txt", std::ios::app);
        log_file_.setf(std::ios::unitbuf);

        if (!log_file_.is_open())
        {
            throw std::runtime_error("Failed to open log file.");
        }

        if (PQstatus(conn) != CONNECTION_OK)
        {
            log_file_ << "SqlCommander. != CONNECTION_OK";
        }
    }
    catch (const std::exception& e)
    {
        log_file_ << "Exception in SqlCommander SqlCommander(): " << e.what();
    }
}

SqlCommander::~SqlCommander() 
{
    if (conn) 
    {
        log_file_ << "Closing connection with DB";
        PQfinish(conn);
    }

    if (log_file_.is_open())
    {
        log_file_ << "Closing log_file";
        log_file_.close();
    }
}

std::map<std::string, std::string> SqlCommander::load_env(const std::string& filename)
{
    try
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
    catch (const std::exception& e)
    {
        log_file_ << "Exception in SqlCommander load_env: " << e.what();
    }
}

void SqlCommander::ExecuteSqlCommand()
{
    try
    {

    }
    catch (const std::exception& e)
    {
        log_file_ << "Exception in SqlCommander ExecuteSqlCommand: " << e.what();
    }
}
