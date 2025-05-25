#include "SqlCommander.h"

SqlCommander::SqlCommander()
    : log_file_("server_output.txt")
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

std::string SqlCommander::execute_sql_command(const std::string& type, const std::string& payload)
{
    try
    {
        if (type == "buyersAdd")
        {
            add_buyers(payload);
        }
        else if (type == "suppliersAdd")
        {
            //add_suppliers(request_);
        }
        else if (type == "productsAdd")
        {
            //add_products(request_);
        }
        else if (type == "warehousesAdd")
        {
            //add_warehouses(request_);
        }
        else if (type == "buyersGet")
        {
            //return get_buyers();
        }
        else if (type == "suppliersGet")
        {
            //return get_suppliers();
        }
        else if (type == "productsGet")
        {
            //return get_products();
        }
        else if (type == "warehousesGet")
        {
            //return get_warehouses();
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

void SqlCommander::add_buyers(const std::string& payload) 
{
    try 
    {
        // Парсим джейсон
        auto j = nlohmann::json::parse(payload);

        // Извлекаем поля из json
        std::string name = j.value("name", "");
        std::string address = j.value("address", "");
        std::string email = j.value("email", "");
        std::string phone = j.value("phone", "");
        std::string tin = j.value("tin", "");
        std::string bankDetails = j.value("bankDetails", "");
        std::string note = j.value("note", "");

        const char* sql =
            "INSERT INTO buyers_suppliers "
            "(name, address, email, phone, tin, bank_details, note, sup) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8);";

        const char* params[8] = {
            name.c_str(),
            address.c_str(),
            email.c_str(),
            phone.c_str(),
            tin.c_str(),
            bankDetails.c_str(),
            note.c_str(),
            "false"
        };

        PGresult* res = PQexecParams(
            conn_,               // соединение
            sql,                 // сам запрос
            8,                   // количество параметров
            nullptr,             // типы параметров
            params,              // сами значения
            nullptr,             // длина
            nullptr,             // формат
            0                    // результат в текстовом формате
        );

        ExecStatusType status = PQresultStatus(res);
        const char* errMsg = PQresultErrorMessage(res);

        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            log_file_.log("add_buyers failed: {}", PQerrorMessage(conn_));
            log_file_.log("add_buyers failed: {}", PQresultErrorMessage(res));
        }
        else 
        {
            std::cout << "add_buyers succeeded\n";
            log_file_.log("add_buyers succeeded");
        }

        PQclear(res);
    }
    catch (const nlohmann::json::parse_error& e) 
    {
        std::cout << "JSON parse error in add_buyers: {}\n";
        log_file_.log("JSON parse error in add_buyers: {}", e.what());
    }
    catch (const std::exception& e) 
    {
        std::cout << "Exception in add_buyers: {}\n";
        log_file_.log("Exception in add_buyers: {}", e.what());
    }
}
