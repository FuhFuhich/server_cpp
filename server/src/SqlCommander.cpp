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

std::string SqlCommander::safe_string(const char* str)
{
    if (!str)
    {
        return "";
    }

    std::string result;

    for (const char* p = str; *p; ++p) 
    {
        unsigned char c = static_cast<unsigned char>(*p);

        if (c >= 32 && c <= 126) 
        {
            result += c;
        }
    }

    return result;
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

std::string SqlCommander::execute_sql_command(const std::string& type, const std::string& profile_id, const std::string& payload)
{
    try
    {
        if (type == "buyersAdd")
        {
            add_buyers(profile_id, payload);
        }
        else if (type == "suppliersAdd")
        {
            add_suppliers(profile_id, payload);
        }
        else if (type == "productsAdd")
        {
            add_products(profile_id, payload);
        }
        else if (type == "warehousesAdd")
        {
            add_warehouses(profile_id, payload);
        }
        else if (type == "buyersGet")
        {
            return get_buyers(profile_id);
        }
        else if (type == "suppliersGet")
        {
            return get_suppliers(profile_id);
        }
        else if (type == "productsGet")
        {
            return get_products(profile_id);
        }
        else if (type == "warehousesGet")
        {
            return get_warehouses(profile_id);
        }
        else if (type == "buyersUpdate")
        {
            update_buyers(profile_id, payload);
        }
        else if (type == "suppliersUpdate")
        {
            update_suppliers(profile_id, payload);
        }
        else if (type == "productsUpdate")
        {
            update_products(profile_id, payload);
        }
        else if (type == "warehousesUpdate")
        {
            update_warehouses(profile_id, payload);
        }
        else if (type == "buyersDelete")
        {
            delete_buyers(profile_id, payload);
        }
        else if (type == "suppliersDelete")
        {
            delete_suppliers(profile_id, payload);
        }
        else if (type == "productsDelete")
        {
            delete_products(profile_id, payload);
        }
        else if (type == "warehousesDelete")
        {
            delete_warehouses(profile_id, payload);
        }
        else if (type == "registration")
        {
            return registration(payload);
        }
        else if (type == "login")
        {
            return login(payload);
        }
        else if (type == "profileGet")
        {
            return get_profile(profile_id);
        }
        else if (type == "profileUpdate")
        {
            profile_update(profile_id, payload);
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

void SqlCommander::add_buyers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string name = safe_string(req.value("name", "").c_str());
    std::string address = safe_string(req.value("address", "").c_str());
    std::string email = safe_string(req.value("email", "").c_str());
    std::string phone = safe_string(req.value("phone", "").c_str());
    std::string tin = safe_string(req.value("tin", "").c_str());
    std::string bankDetails = safe_string(req.value("bankDetails", "").c_str());
    std::string note = safe_string(req.value("note", "").c_str());

    log_file_.log("=== ADDING BUYER ===");
    log_file_.log("Profile ID: {}, Name: {}", profile_id, name);

    static const char* sql =
        "INSERT INTO buyers_suppliers (name, address, email, phone, tin, bank_details, note, sup) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, false) RETURNING id_buyer;";

    const char* params[7] = {
        name.c_str(), address.c_str(), email.c_str(), phone.c_str(),
        tin.c_str(), bankDetails.c_str(), note.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        7,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_buyers: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_buyers: {}", err);
        PQclear(res);
        return;
    }

    int buyer_id = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_file_.log("Buyer inserted with ID: {}", buyer_id);

    static const char* sql_link =
        "INSERT INTO user_buyers_suppliers (id_user, id_buyer) VALUES ($1, $2);";

    std::string buyer_id_str = std::to_string(buyer_id);
    const char* link_params[2] = {
        profile_id.c_str(),
        buyer_id_str.c_str()
    };

    log_file_.log("Inserting link: user_id={}, buyer_id={}", profile_id, buyer_id);

    res = PQexecParams(
        conn_,
        sql_link,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_buyers link: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_buyers link: {}", err);
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("Buyer and link added successfully for profile: {}", profile_id);
}

void SqlCommander::add_suppliers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string name = safe_string(req.value("name", "").c_str());
    std::string address = safe_string(req.value("address", "").c_str());
    std::string email = safe_string(req.value("email", "").c_str());
    std::string phone = safe_string(req.value("phone", "").c_str());
    std::string tin = safe_string(req.value("tin", "").c_str());
    std::string bankDetails = safe_string(req.value("bankDetails", "").c_str());
    std::string note = safe_string(req.value("note", "").c_str());

    log_file_.log("=== ADDING SUPPLIER ===");
    log_file_.log("Profile ID: {}, Name: {}", profile_id, name);

    static const char* sql =
        "INSERT INTO buyers_suppliers (name, address, email, phone, tin, bank_details, note, sup) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, true) RETURNING id_buyer;";

    const char* params[7] = {
        name.c_str(), address.c_str(), email.c_str(), phone.c_str(),
        tin.c_str(), bankDetails.c_str(), note.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        7,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_suppliers: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_suppliers: {}", err);
        PQclear(res);
        return;
    }

    int supplier_id = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_file_.log("Supplier inserted with ID: {}", supplier_id);

    static const char* sql_link =
        "INSERT INTO user_buyers_suppliers (id_user, id_buyer) VALUES ($1, $2);";

    std::string supplier_id_str = std::to_string(supplier_id);
    const char* link_params[2] = {
        profile_id.c_str(),
        supplier_id_str.c_str()
    };

    log_file_.log("Inserting link: user_id={}, supplier_id={}", profile_id, supplier_id);

    res = PQexecParams(
        conn_,
        sql_link,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_suppliers link: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_suppliers link: {}", err);
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("Supplier and link added successfully for profile: {}", profile_id);
}

void SqlCommander::add_products(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string name = safe_string(req.value("name", "").c_str());
    std::string description = safe_string(req.value("description", "").c_str());
    std::string barcode = safe_string(req.value("barcode", "").c_str());

    std::string imageUri = "";
    if (!req["imageUri"].is_null())
    {
        imageUri = safe_string(req["imageUri"].get<std::string>().c_str());
    }

    int quantity = req.value("quantity", 0);
    std::string warehouse = safe_string(req.value("warehouse", "").c_str());

    log_file_.log("=== ADDING PRODUCT ===");
    log_file_.log("Profile ID: {}, Name: {}", profile_id, name);

    static const char* sql =
        "INSERT INTO product (name, description, barcode, imageuri, quantity, warehouse) "
        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id_product;";

    std::string quantityStr = std::to_string(quantity);
    const char* params[6] = {
        name.c_str(),
        description.c_str(),
        barcode.c_str(),
        imageUri.empty() ? nullptr : imageUri.c_str(),
        quantityStr.c_str(),
        warehouse.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        6,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_products: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_products: {}", err);
        PQclear(res);
        return;
    }

    int product_id = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_file_.log("Product inserted with ID: {}", product_id);

    static const char* sql_link =
        "INSERT INTO user_product (id_user, id_product) VALUES ($1, $2);";

    std::string product_id_str = std::to_string(product_id);
    const char* link_params[2] = {
        profile_id.c_str(),
        product_id_str.c_str()
    };

    log_file_.log("Inserting link: user_id={}, product_id={}", profile_id, product_id);

    res = PQexecParams(
        conn_,
        sql_link,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_products link: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_products link: {}", err);
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("Product and link added successfully for profile: {}", profile_id);
}

void SqlCommander::add_warehouses(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string warehouseName = safe_string(req.value("warehousesName", "").c_str());

    log_file_.log("=== ADDING WAREHOUSE ===");
    log_file_.log("Profile ID: {}, Name: {}", profile_id, warehouseName);

    static const char* sql =
        "INSERT INTO warehouses (warehouses_name) "
        "VALUES ($1) RETURNING id_warehouse;";

    const char* params[1] = {
        warehouseName.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_warehouses: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_warehouses: {}", err);
        PQclear(res);
        return;
    }

    int warehouse_id = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    log_file_.log("Warehouse inserted with ID: {}", warehouse_id);

    static const char* sql_link =
        "INSERT INTO user_warehouse (id_user, id_warehouse) VALUES ($1, $2);";

    std::string warehouse_id_str = std::to_string(warehouse_id);
    const char* link_params[2] = {
        profile_id.c_str(),
        warehouse_id_str.c_str()
    };

    log_file_.log("Inserting link: user_id={}, warehouse_id={}", profile_id, warehouse_id);

    res = PQexecParams(
        conn_,
        sql_link,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in add_warehouses link: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in add_warehouses link: {}", err);
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("Warehouse and link added successfully for profile: {}", profile_id);
}

std::string SqlCommander::registration(const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    std::string login = req.value("login", "");
    std::string password = req.value("password", "");

    log_file_.log("=== REGISTRATION ATTEMPT ===");
    log_file_.log("Login: {}", login);

    if (login.empty() || password.empty()) 
    {
        log_file_.log("Registration failed: empty login or password");
        return "profileGet " + nlohmann::json{ {"error", "Заполните все поля"} }.dump();
    }

    if (login.length() < 3) 
    {
        log_file_.log("Registration failed: login too short");
        return "profileGet " + nlohmann::json{ {"error", "Логин должен содержать минимум 3 символа"} }.dump();
    }

    if (password.length() < 3) 
    {
        log_file_.log("Registration failed: password too short");
        return "profileGet " + nlohmann::json{ {"error", "Пароль должен содержать минимум 3 символа"} }.dump();
    }

    static const char* check_sql =
        "SELECT login FROM profile WHERE login = $1;";

    const char* check_params[1] = { login.c_str() };

    PGresult* check_res = PQexecParams(
        conn_,
        check_sql,
        1,
        nullptr,
        check_params,
        nullptr,
        nullptr,
        0
    );

    if (!check_res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in registration check: {}", err);
        return "profileGet " + nlohmann::json{ {"error", "Ошибка базы данных"} }.dump();
    }

    if (PQresultStatus(check_res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(check_res);
        log_file_.log("Query error in registration check: {}", err);
        PQclear(check_res);
        return "profileGet " + nlohmann::json{ {"error", "Ошибка запроса"} }.dump();
    }

    if (PQntuples(check_res) > 0) 
    {
        log_file_.log("Registration failed: login already exists: {}", login);
        PQclear(check_res);
        return "profileGet " + nlohmann::json{ {"error", "Такой логин уже существует"} }.dump();
    }

    PQclear(check_res);
    log_file_.log("Login is available, creating new user");

    static const char* insert_sql =
        "INSERT INTO profile (login, password) "
        "VALUES ($1, $2) "
        "RETURNING id_user;";

    const char* insert_params[2] = {
        login.c_str(),
        password.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        insert_sql,
        2,
        nullptr,
        insert_params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in registration insert: {}", err);
        return "profileGet " + nlohmann::json{ {"error", "Ошибка создания пользователя"} }.dump();
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in registration insert: {}", err);
        PQclear(res);
        return "profileGet " + nlohmann::json{ {"error", "Ошибка создания пользователя"} }.dump();
    }

    int new_id = std::atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    nlohmann::json resp = {
        {"id_user", new_id},
        {"login", login},
        {"firstName", ""},
        {"lastName", ""},
        {"phone", ""},
        {"email", ""},
        {"photoUri", ""}
    };

    log_file_.log("Registration successful for user: {}, ID: {}", login, new_id);
    log_file_.log("Response: {}", resp.dump());
    return "profileGet " + resp.dump();
}


std::string SqlCommander::login(const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    std::string login = req.value("login", "");
    std::string password = req.value("password", "");

    static const char* sql =
        "SELECT id_user, first_name, last_name, login, phone, email, photouri "
        "FROM profile "
        "WHERE login = $1 AND password = $2;";

    const char* params[2] = {
        login.c_str(),
        password.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        2,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res)
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Database error in login: {}", err);
        return "profileGet {\"error\":\"Database error\"}";
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Query error in login: {}", err);
        PQclear(res);
        return "profileGet {\"error\":\"Query error\"}";
    }

    if (PQntuples(res) == 0)
    {
        log_file_.log("Invalid credentials for user: {}", login);
        PQclear(res);
        return "profileGet {\"error\":\"Invalid credentials\"}";
    }

    int id = std::atoi(PQgetvalue(res, 0, 0));
    std::string fn = safe_string(PQgetvalue(res, 0, 1));
    std::string ln = safe_string(PQgetvalue(res, 0, 2));
    std::string lg = safe_string(PQgetvalue(res, 0, 3));
    std::string ph = safe_string(PQgetvalue(res, 0, 4));
    std::string em = safe_string(PQgetvalue(res, 0, 5));
    std::string pu = safe_string(PQgetvalue(res, 0, 6));
    PQclear(res);

    std::string response = "profileGet {";
    response += "\"id_user\":" + std::to_string(id) + ",";
    response += "\"firstName\":\"" + fn + "\",";
    response += "\"lastName\":\"" + ln + "\",";
    response += "\"login\":\"" + lg + "\",";
    response += "\"phone\":\"" + ph + "\",";
    response += "\"email\":\"" + em + "\",";
    response += "\"photoUri\":\"" + pu + "\"";
    response += "}";

    log_file_.log("SqlCommander login successful for user: {}", login);
    log_file_.log("SqlCommander login Response: {}", response);
    return response;
}

std::string SqlCommander::get_profile(const std::string& profile_id)
{
    if (profile_id.empty()) 
    {
        return "profileGet " + nlohmann::json{ {"error", "Profile ID is required"} }.dump();
    }

    static const char* sql =
        "SELECT id_user, first_name, last_name, login, phone, email, photouri "
        "FROM profile "
        "WHERE id_user = $1;";

    const char* params[1] = { profile_id.c_str() };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Exception in SqlCommander get_profile: {}", err);
        return "profileGet " + nlohmann::json{ {"error", err} }.dump();
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Exception in SqlCommander get_profile: {}", err);
        PQclear(res);
        return "profileGet " + nlohmann::json{ {"error", err} }.dump();
    }

    if (PQntuples(res) == 0) 
    {
        std::string err = nlohmann::json{ {"error", "Profile not found"} }.dump();
        log_file_.log("Exception in SqlCommander get_profile: {}", err);
        PQclear(res);
        return "profileGet " + err;
    }

    int id = std::atoi(PQgetvalue(res, 0, 0));
    const char* fn = PQgetvalue(res, 0, 1);
    const char* ln = PQgetvalue(res, 0, 2);
    const char* lg = PQgetvalue(res, 0, 3);
    const char* ph = PQgetvalue(res, 0, 4);
    const char* em = PQgetvalue(res, 0, 5);
    const char* pu = PQgetvalue(res, 0, 6);
    PQclear(res);

    nlohmann::json resp = {
        {"id_user", id},
        {"firstName", fn ? fn : ""},
        {"lastName", ln ? ln : ""},
        {"login", lg ? lg : ""},
        {"phone", ph ? ph : ""},
        {"email", em ? em : ""},
        {"photoUri", pu ? pu : ""}
    };

    log_file_.log("profileGet: {}", resp.dump());
    return "profileGet " + resp.dump();
}

void SqlCommander::profile_update(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string firstName = safe_string(req.value("firstName", "").c_str());
    std::string lastName = safe_string(req.value("lastName", "").c_str());
    std::string login = safe_string(req.value("login", "").c_str());
    std::string phone = safe_string(req.value("phone", "").c_str());
    std::string email = safe_string(req.value("email", "").c_str());
    std::string photoUri = safe_string(req.value("photoUri", "").c_str());

    static const char* sql =
        "UPDATE profile SET first_name = $1, last_name = $2, login = $3, phone = $4, email = $5, photouri = $6 WHERE id_user = $7;";

    const char* params[7] = {
        firstName.c_str(),
        lastName.c_str(),
        login.c_str(),
        phone.c_str(),
        email.c_str(),
        photoUri.c_str(),
        profile_id.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        7,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("Exception in SqlCommander profile_update Database error in profile_update: {}", err);
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Exception in SqlCommander profile_update Query error in profile_update: {}", err);
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("profile_update Profile updated successfully for user: {}", profile_id);
}

void SqlCommander::delete_buyers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int buyer_id = req.value("id", 0);

    if (buyer_id == 0) 
    {
        log_file_.log("delete_buyers: Invalid buyer_id");
        return;
    }

    static const char* delete_link_sql =
        "DELETE FROM profile_buyers WHERE profile_id = $1 AND buyer_id = $2;";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(buyer_id).c_str()
    };

    PGresult* res = PQexecParams(conn_, delete_link_sql, 2, nullptr, link_params, nullptr, nullptr, 0);
    if (res) PQclear(res);

    static const char* delete_buyer_sql =
        "DELETE FROM buyers_suppliers WHERE id_buyer = $1 AND sup = false;";

    const char* buyer_params[1] = { std::to_string(buyer_id).c_str() };
    res = PQexecParams(conn_, delete_buyer_sql, 1, nullptr, buyer_params, nullptr, nullptr, 0);

    if (!res) 
    {
        log_file_.log("delete_buyers: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("delete_buyers: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("delete_buyers: Buyer deleted successfully, id: {}", buyer_id);
}

void SqlCommander::delete_suppliers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int supplier_id = req.value("id", 0);

    if (supplier_id == 0) 
    {
        log_file_.log("delete_suppliers: Invalid supplier_id");
        return;
    }

    static const char* delete_link_sql =
        "DELETE FROM profile_suppliers WHERE profile_id = $1 AND supplier_id = $2;";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(supplier_id).c_str()
    };

    PGresult* res = PQexecParams(conn_, delete_link_sql, 2, nullptr, link_params, nullptr, nullptr, 0);
    if (res) PQclear(res);

    static const char* delete_supplier_sql =
        "DELETE FROM buyers_suppliers WHERE id_buyer = $1 AND sup = true;";

    const char* supplier_params[1] = { std::to_string(supplier_id).c_str() };
    res = PQexecParams(conn_, delete_supplier_sql, 1, nullptr, supplier_params, nullptr, nullptr, 0);

    if (!res) 
    {
        log_file_.log("delete_suppliers: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("delete_suppliers: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("delete_suppliers: Supplier deleted successfully, id: {}", supplier_id);
}

void SqlCommander::delete_products(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int product_id = req.value("id", 0);

    if (product_id == 0)
    {
        log_file_.log("delete_products: Invalid product_id");
        return;
    }

    static const char* delete_link_sql =
        "DELETE FROM user_product WHERE id_user = $1 AND id_product = $2;";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(product_id).c_str()
    };

    PGresult* res = PQexecParams(conn_, delete_link_sql, 2, nullptr, link_params, nullptr, nullptr, 0);
    if (res) PQclear(res);

    static const char* delete_product_sql =
        "DELETE FROM product WHERE id_product = $1;";

    const char* product_params[1] = { std::to_string(product_id).c_str() };
    res = PQexecParams(conn_, delete_product_sql, 1, nullptr, product_params, nullptr, nullptr, 0);

    if (!res)
    {
        log_file_.log("delete_products: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        log_file_.log("delete_products: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("delete_products: Product deleted successfully, id: {}", product_id);
}

void SqlCommander::delete_warehouses(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int warehouse_id = req.value("id", 0);

    if (warehouse_id == 0)
    {
        log_file_.log("delete_warehouses: Invalid warehouse_id");
        return;
    }

    log_file_.log("=== DELETING WAREHOUSE ===");
    log_file_.log("Profile ID: {}, Warehouse ID: {}", profile_id, warehouse_id);

    std::string warehouse_id_str = std::to_string(warehouse_id);

    static const char* delete_product_links_sql = R"(
            DELETE FROM user_product 
            WHERE id_product IN (
                SELECT p.id_product 
                FROM product p 
                INNER JOIN warehouses w ON p.warehouse = w.warehouses_name 
                WHERE w.id_warehouse = $1
            );
        )";

    const char* warehouse_params1[1] = { warehouse_id_str.c_str() };
    PGresult* res = PQexecParams(conn_, delete_product_links_sql, 1, nullptr, warehouse_params1, nullptr, nullptr, 0);

    if (res)
    {
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::string err = PQresultErrorMessage(res);
            log_file_.log("delete_warehouses: Error deleting product links: {}", err);
        }
        else {
            int affected_rows = std::atoi(PQcmdTuples(res));
            log_file_.log("Deleted {} product links", affected_rows);
        }
        PQclear(res);
    }

    static const char* delete_products_sql = R"(
            DELETE FROM product 
            WHERE warehouse = (
                SELECT warehouses_name 
                FROM warehouses 
                WHERE id_warehouse = $1
            );
        )";

    const char* warehouse_params2[1] = { warehouse_id_str.c_str() };
    res = PQexecParams(conn_, delete_products_sql, 1, nullptr, warehouse_params2, nullptr, nullptr, 0);

    if (res)
    {
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::string err = PQresultErrorMessage(res);
            log_file_.log("delete_warehouses: Error deleting products: {}", err);
        }
        else {
            int affected_rows = std::atoi(PQcmdTuples(res));
            log_file_.log("Deleted {} products from warehouse", affected_rows);
        }
        PQclear(res);
    }

    static const char* delete_link_sql =
        "DELETE FROM user_warehouse WHERE id_user = $1 AND id_warehouse = $2;";

    const char* link_params[2] = {
        profile_id.c_str(),
        warehouse_id_str.c_str()
    };

    res = PQexecParams(conn_, delete_link_sql, 2, nullptr, link_params, nullptr, nullptr, 0);

    if (res)
    {
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            std::string err = PQresultErrorMessage(res);
            log_file_.log("delete_warehouses link error: {}", err);
        }
        PQclear(res);
    }

    static const char* delete_warehouse_sql =
        "DELETE FROM warehouses WHERE id_warehouse = $1;";

    const char* warehouse_params3[1] = { warehouse_id_str.c_str() };
    res = PQexecParams(conn_, delete_warehouse_sql, 1, nullptr, warehouse_params3, nullptr, nullptr, 0);

    if (!res)
    {
        log_file_.log("delete_warehouses: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        log_file_.log("delete_warehouses: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("delete_warehouses: Warehouse and related products deleted successfully, id: {}", warehouse_id);
}

void SqlCommander::update_buyers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int buyer_id = req.value("id", 0);

    if (buyer_id == 0) 
    {
        log_file_.log("update_buyers: Invalid buyer_id");
        return;
    }

    std::string name = safe_string(req.value("name", "").c_str());
    std::string address = safe_string(req.value("address", "").c_str());
    std::string email = safe_string(req.value("email", "").c_str());
    std::string phone = safe_string(req.value("phone", "").c_str());
    std::string tin = safe_string(req.value("tin", "").c_str());
    std::string bankDetails = safe_string(req.value("bankDetails", "").c_str());
    std::string note = safe_string(req.value("note", "").c_str());

    static const char* sql =
        "UPDATE buyers_suppliers SET name = $1, address = $2, email = $3, phone = $4, "
        "tin = $5, bank_details = $6, note = $7 WHERE id_buyer = $8 AND sup = false;";

    const char* params[8] = {
        name.c_str(), address.c_str(), email.c_str(), phone.c_str(),
        tin.c_str(), bankDetails.c_str(), note.c_str(), std::to_string(buyer_id).c_str()
    };

    PGresult* res = PQexecParams(
        conn_, 
        sql, 
        8, 
        nullptr, 
        params, 
        nullptr, 
        nullptr, 
        0
    );

    if (!res) 
    {
        log_file_.log("update_buyers: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("update_buyers: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("update_buyers: Buyer updated successfully, id: {}", buyer_id);
}

void SqlCommander::update_suppliers(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int supplier_id = req.value("id", 0);

    if (supplier_id == 0)
    {
        log_file_.log("update_suppliers: Invalid supplier_id");
        return;
    }

    std::string name = safe_string(req.value("name", "").c_str());
    std::string address = safe_string(req.value("address", "").c_str());
    std::string email = safe_string(req.value("email", "").c_str());
    std::string phone = safe_string(req.value("phone", "").c_str());
    std::string tin = safe_string(req.value("tin", "").c_str());
    std::string bankDetails = safe_string(req.value("bankDetails", "").c_str());
    std::string note = safe_string(req.value("note", "").c_str());

    static const char* sql =
        "UPDATE buyers_suppliers SET name = $1, address = $2, email = $3, phone = $4, "
        "tin = $5, bank_details = $6, note = $7 WHERE id_buyer = $8 AND sup = true;";

    const char* params[8] = {
        name.c_str(), address.c_str(), email.c_str(), phone.c_str(),
        tin.c_str(), bankDetails.c_str(), note.c_str(), std::to_string(supplier_id).c_str()
    };

    PGresult* res = PQexecParams(
        conn_, 
        sql, 
        8, 
        nullptr, 
        params, 
        nullptr, 
        nullptr, 
        0
    );

    if (!res)
    {
        log_file_.log("update_suppliers: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        log_file_.log("update_suppliers: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("update_suppliers: Supplier updated successfully, id: {}", supplier_id);
}

void SqlCommander::update_products(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int product_id = req.value("id", 0);

    if (product_id == 0) 
    {
        log_file_.log("update_products: Invalid product_id");
        return;
    }

    std::string name = safe_string(req.value("name", "").c_str());
    std::string description = safe_string(req.value("description", "").c_str());
    std::string barcode = safe_string(req.value("barcode", "").c_str());
    std::string imageUri = req.value("imageUri", "") == "null" ? "" : safe_string(req.value("imageUri", "").c_str());
    int quantity = req.value("quantity", 0);
    std::string warehouse = safe_string(req.value("warehouse", "").c_str());

    static const char* sql =
        "UPDATE product SET name = $1, description = $2, barcode = $3, imageuri = $4, "
        "quantity = $5, warehouse = $6 WHERE id_product = $7;";

    std::string quantityStr = std::to_string(quantity);
    const char* params[7] = {
        name.c_str(), description.c_str(), barcode.c_str(),
        imageUri.empty() ? nullptr : imageUri.c_str(),
        quantityStr.c_str(), warehouse.c_str(), std::to_string(product_id).c_str()
    };

    PGresult* res = PQexecParams(
        conn_, 
        sql, 
        7, 
        nullptr, 
        params, 
        nullptr, 
        nullptr, 
        0
    );

    if (!res) 
    {
        log_file_.log("update_products: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("update_products: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("update_products: Product updated successfully, id: {}", product_id);
    return;
}

void SqlCommander::update_warehouses(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    int warehouse_id = req.value("id", 0);

    if (warehouse_id == 0) 
    {
        log_file_.log("update_warehouses: Invalid warehouse_id");
        return;
    }

    std::string warehouseName = safe_string(req.value("warehousesName", "").c_str());

    static const char* sql =
        "UPDATE warehouses SET warehouse_name = $1 WHERE id_warehouse = $2;";

    const char* params[2] = {
        warehouseName.c_str(),
        std::to_string(warehouse_id).c_str()
    };

    PGresult* res = PQexecParams(
        conn_, 
        sql, 
        2, 
        nullptr, 
        params, 
        nullptr, 
        nullptr, 
        0
    );

    if (!res) 
    {
        log_file_.log("update_warehouses: Database error: {}", PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("update_warehouses: Query error: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    PQclear(res);
    log_file_.log("update_warehouses: Warehouse updated successfully, id: {}", warehouse_id);
}

std::string SqlCommander::get_buyers(const std::string& profile_id)
{
    if (profile_id.empty()) 
    {
        log_file_.log("get_buyers: Profile ID is empty");
        return "buyersGet []";
    }

    static const char* sql = R"(
        SELECT bs.id_buyer, bs.name, bs.address, bs.email, bs.phone, 
               bs.tin, bs.bank_details, bs.note
        FROM buyers_suppliers bs
        INNER JOIN user_buyers_suppliers ubs ON bs.id_buyer = ubs.id_buyer
        WHERE ubs.id_user = $1 AND bs.sup = false
        ORDER BY bs.name;
    )";

    const char* params[1] = { profile_id.c_str() };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("get_buyers: Database error: {}", err);
        return "buyersGet []";
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("get_buyers: Query error: {}", err);
        PQclear(res);
        return "buyersGet []";
    }

    int rows = PQntuples(res);
    std::string jsonArray = "[";

    for (int i = 0; i < rows; i++) 
    {
        if (i > 0)
        {
            jsonArray += ",";
        }

        int id = std::atoi(PQgetvalue(res, i, 0));
        std::string name = safe_string(PQgetvalue(res, i, 1));
        std::string address = safe_string(PQgetvalue(res, i, 2));
        std::string email = safe_string(PQgetvalue(res, i, 3));
        std::string phone = safe_string(PQgetvalue(res, i, 4));
        std::string tin = safe_string(PQgetvalue(res, i, 5));
        std::string bankDetails = safe_string(PQgetvalue(res, i, 6));
        std::string note = safe_string(PQgetvalue(res, i, 7));

        jsonArray += "{";
        jsonArray += "\"id\":" + std::to_string(id) + ",";
        jsonArray += "\"name\":\"" + name + "\",";
        jsonArray += "\"address\":\"" + address + "\",";
        jsonArray += "\"email\":\"" + email + "\",";
        jsonArray += "\"phone\":\"" + phone + "\",";
        jsonArray += "\"tin\":\"" + tin + "\",";
        jsonArray += "\"bankDetails\":\"" + bankDetails + "\",";
        jsonArray += "\"note\":\"" + note + "\"";
        jsonArray += "}";
    }

    jsonArray += "]";
    PQclear(res);

    log_file_.log("get_buyers: Found {} buyers for profile {}", rows, profile_id);
    return "buyersGet " + jsonArray;
}

std::string SqlCommander::get_suppliers(const std::string& profile_id)
{
    if (profile_id.empty()) 
    {
        log_file_.log("get_suppliers: Profile ID is empty");
        return "suppliersGet []";
    }

    static const char* sql = R"(
        SELECT bs.id_buyer, bs.name, bs.address, bs.email, bs.phone, 
               bs.tin, bs.bank_details, bs.note
        FROM buyers_suppliers bs
        INNER JOIN user_buyers_suppliers ubs ON bs.id_buyer = ubs.id_buyer
        WHERE ubs.id_user = $1 AND bs.sup = true
        ORDER BY bs.name;
    )";

    const char* params[1] = { profile_id.c_str() };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("get_suppliers: Database error: {}", err);
        return "suppliersGet []";
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("get_suppliers: Query error: {}", err);
        PQclear(res);
        return "suppliersGet []";
    }

    int rows = PQntuples(res);
    std::string jsonArray = "[";

    for (int i = 0; i < rows; i++) 
    {
        if (i > 0)
        {
            jsonArray += ",";
        }

        int id = std::atoi(PQgetvalue(res, i, 0));
        std::string name = safe_string(PQgetvalue(res, i, 1));
        std::string address = safe_string(PQgetvalue(res, i, 2));
        std::string email = safe_string(PQgetvalue(res, i, 3));
        std::string phone = safe_string(PQgetvalue(res, i, 4));
        std::string tin = safe_string(PQgetvalue(res, i, 5));
        std::string bankDetails = safe_string(PQgetvalue(res, i, 6));
        std::string note = safe_string(PQgetvalue(res, i, 7));

        jsonArray += "{";
        jsonArray += "\"id\":" + std::to_string(id) + ",";
        jsonArray += "\"name\":\"" + name + "\",";
        jsonArray += "\"address\":\"" + address + "\",";
        jsonArray += "\"email\":\"" + email + "\",";
        jsonArray += "\"phone\":\"" + phone + "\",";
        jsonArray += "\"tin\":\"" + tin + "\",";
        jsonArray += "\"bankDetails\":\"" + bankDetails + "\",";
        jsonArray += "\"note\":\"" + note + "\"";
        jsonArray += "}";
    }

    jsonArray += "]";
    PQclear(res);

    log_file_.log("get_suppliers: Found {} suppliers for profile {}", rows, profile_id);
    return "suppliersGet " + jsonArray;
}

std::string SqlCommander::get_products(const std::string& profile_id)
{
    if (profile_id.empty()) 
    {
        log_file_.log("get_products: Profile ID is empty");
        return "productsGet []";
    }

    static const char* sql = R"(
        SELECT p.id_product, p.name, p.description, p.barcode, p.imageuri, p.quantity, p.warehouse
        FROM product p
        INNER JOIN user_product up ON p.id_product = up.id_product
        WHERE up.id_user = $1
        ORDER BY p.name;
    )";

    const char* params[1] = { profile_id.c_str() };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("get_products: Database error: {}", err);
        return "productsGet []";
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("get_products: Query error: {}", err);
        PQclear(res);
        return "productsGet []";
    }

    int rows = PQntuples(res);
    std::string jsonArray = "[";

    for (int i = 0; i < rows; i++) 
    {
        if (i > 0)
        {
            jsonArray += ",";
        }

        int id = std::atoi(PQgetvalue(res, i, 0));
        std::string name = safe_string(PQgetvalue(res, i, 1));
        std::string description = safe_string(PQgetvalue(res, i, 2));
        std::string barcode = safe_string(PQgetvalue(res, i, 3));
        std::string imageUri = PQgetvalue(res, i, 4) ? safe_string(PQgetvalue(res, i, 4)) : "";
        int quantity = std::atoi(PQgetvalue(res, i, 5));
        std::string warehouse = safe_string(PQgetvalue(res, i, 6));

        jsonArray += "{";
        jsonArray += "\"id\":" + std::to_string(id) + ",";
        jsonArray += "\"name\":\"" + name + "\",";
        jsonArray += "\"description\":\"" + description + "\",";
        jsonArray += "\"barcode\":\"" + barcode + "\",";
        jsonArray += "\"imageUri\":\"" + imageUri + "\",";
        jsonArray += "\"quantity\":" + std::to_string(quantity) + ",";
        jsonArray += "\"warehouse\":\"" + warehouse + "\"";
        jsonArray += "}";
    }

    jsonArray += "]";
    PQclear(res);

    log_file_.log("get_products: Found {} products for profile {}", rows, profile_id);
    return "productsGet " + jsonArray;
}

std::string SqlCommander::get_warehouses(const std::string& profile_id)
{
    if (profile_id.empty()) 
    {
        log_file_.log("get_warehouses: Profile ID is empty");
        return "warehousesGet []";
    }

    static const char* sql = R"(
        SELECT w.id_warehouse, w.warehouses_name
        FROM warehouses w
        INNER JOIN user_warehouse uw ON w.id_warehouse = uw.id_warehouse
        WHERE uw.id_user = $1
        ORDER BY w.warehouses_name;
    )";

    const char* params[1] = { profile_id.c_str() };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        1,
        nullptr,
        params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        std::string err = PQerrorMessage(conn_);
        log_file_.log("get_warehouses: Database error: {}", err);
        return "warehousesGet []";
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("get_warehouses: Query error: {}", err);
        PQclear(res);
        return "warehousesGet []";
    }

    int rows = PQntuples(res);
    std::string jsonArray = "[";

    for (int i = 0; i < rows; i++) 
    {
        if (i > 0)
        {
            jsonArray += ",";
        }

        int id = std::atoi(PQgetvalue(res, i, 0));
        std::string warehouseName = safe_string(PQgetvalue(res, i, 1));

        jsonArray += "{";
        jsonArray += "\"id\":" + std::to_string(id) + ",";
        jsonArray += "\"warehousesName\":\"" + warehouseName + "\"";
        jsonArray += "}";
    }

    jsonArray += "]";
    PQclear(res);

    log_file_.log("get_warehouses: Found {} warehouses for profile {}", rows, profile_id);
    return "warehousesGet " + jsonArray;
}
