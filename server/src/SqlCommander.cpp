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
            //return get_buyers(profile_id);
        }
        else if (type == "suppliersGet")
        {
            //return get_suppliers(profile_id);
        }
        else if (type == "productsGet")
        {
            //return get_products(profile_id);
        }
        else if (type == "warehousesGet")
        {
            //return get_warehouses(profile_id);
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
    // Парсим джейсон
    auto j = nlohmann::json::parse(payload);
    std::string name = j.value("name", "");
    std::string address = j.value("address", "");
    std::string email = j.value("email", "");
    std::string phone = j.value("phone", "");
    std::string tin = j.value("tin", "");
    std::string bankDetails = j.value("bankDetails", "");
    std::string note = j.value("note", "");

    static const char* insert_buyer_sql = R"(
        INSERT INTO buyers_suppliers
         (name, address, email, phone, tin, bank_details, note, sup)
        VALUES ($1,$2,$3,$4,$5,$6,$7,$8)
        RETURNING id_buyer;
    )";

    const char* buyer_params[8] = {
        name.c_str(), address.c_str(), email.c_str(),
        phone.c_str(), tin.c_str(), bankDetails.c_str(),
        note.c_str(), "false"
    };

    PGresult* res = PQexecParams(
        conn_,
        insert_buyer_sql,
        8,
        nullptr,      
        buyer_params,
        nullptr,
        nullptr,
        0             
    );

    if (!res) 
    {
        log_file_.log("add_buyers: PQexecParams returned nullptr: {}",
            PQerrorMessage(conn_));
        return;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        log_file_.log("add_buyers failed: {}",
            PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    char* id_str = PQgetvalue(res, 0, 0);
    int   buyer_id = std::atoi(id_str);
    PQclear(res);

    log_file_.log("add_buyers: new buyer_id = {}", buyer_id);

    static const char* insert_link_sql = R"(
        INSERT INTO profile_buyers (profile_id, buyer_id)
        VALUES ($1,$2);
    )";

    const char* link_params[2] = {
        profile_id.c_str(),
        id_str
    };

    PGresult* link_res = PQexecParams(
        conn_,
        insert_link_sql,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!link_res) 
    {
        log_file_.log("link insert: PQexecParams returned nullptr: {}", PQerrorMessage(conn_));
    }
    else if (PQresultStatus(link_res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("link insert failed: {}", PQresultErrorMessage(link_res));
    }
    else 
    {
        log_file_.log("link insert succeeded: profile_id={}, buyer_id={}", profile_id, buyer_id);
    }

    PQclear(link_res);
}

void SqlCommander::add_suppliers(const std::string& profile_id, const std::string& payload)
{
    // Парсим джейсон
    auto j = nlohmann::json::parse(payload);
    std::string name = j.value("name", "");
    std::string address = j.value("address", "");
    std::string email = j.value("email", "");
    std::string phone = j.value("phone", "");
    std::string tin = j.value("tin", "");
    std::string bankDetails = j.value("bankDetails", "");
    std::string note = j.value("note", "");

    static const char* insert_supplier_sql = R"(
        INSERT INTO buyers_suppliers
          (name, address, email, phone, tin, bank_details, note, sup)
        VALUES ($1,$2,$3,$4,$5,$6,$7,$8)
        RETURNING id_buyer;
    )";

    const char* supplier_params[8] = {
        name.c_str(), address.c_str(), email.c_str(),
        phone.c_str(), tin.c_str(), bankDetails.c_str(),
        note.c_str(), "true"
    };

    PGresult* res = PQexecParams(
        conn_,
        insert_supplier_sql,
        8,
        nullptr,
        supplier_params,
        nullptr,
        nullptr,
        0
    );

    if (!res) 
    {
        log_file_.log("add_suppliers: PQexecParams returned nullptr: {}", PQerrorMessage(conn_));
        return;
    }
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        log_file_.log("add_suppliers failed: {}", PQresultErrorMessage(res));
        PQclear(res);
        return;
    }

    char* id_str = PQgetvalue(res, 0, 0);
    int   supplier_id = std::atoi(id_str);
    PQclear(res);

    log_file_.log("add_suppliers: new supplier_id = {}", supplier_id);

    static const char* insert_link_sql = R"(
        INSERT INTO profile_suppliers (profile_id, supplier_id)
        VALUES ($1,$2);
    )";

    const char* link_params[2] = {
        profile_id.c_str(),
        id_str
    };

    PGresult* link_res = PQexecParams(
        conn_,
        insert_link_sql,
        2,
        nullptr,
        link_params,
        nullptr,
        nullptr,
        0
    );

    if (!link_res) 
    {
        log_file_.log("add_suppliers: link insert: PQexecParams returned nullptr: {}", PQerrorMessage(conn_));
    }
    else if (PQresultStatus(link_res) != PGRES_COMMAND_OK) 
    {
        log_file_.log("add_suppliers: link insert failed: {}", PQresultErrorMessage(link_res));
    }
    else 
    {
        log_file_.log("add_suppliers: link insert succeeded: profile_id={}, supplier_id={}", profile_id, supplier_id);
    }

    PQclear(link_res);
}

std::string SqlCommander::registration(const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);
    std::string login = req.value("login", "");
    std::string password = req.value("password", "");

    static const char* sql =
        "INSERT INTO profile (login, password) "
        "VALUES ($1, $2) "
        "RETURNING id_user;";

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

        log_file_.log("Exception in SqlCommander registration: {}", err);
        return "profileGet " + nlohmann::json{ {"error", err} }.dump();
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        std::string err = PQresultErrorMessage(res);
        log_file_.log("Exception in SqlCommander registration PQresultStatus(res) != PGRES_TUPLES_OK: {}", err);
        PQclear(res);
        return "profileGet " + nlohmann::json{ {"error", err} }.dump();
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

    log_file_.log("profileGet: {}", resp.dump());
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

void SqlCommander::add_products(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string name = safe_string(req.value("name", "").c_str());
    std::string description = safe_string(req.value("description", "").c_str());
    std::string barcode = safe_string(req.value("barcode", "").c_str());
    std::string imageUri = req.value("imageUri", "") == "null" ? "" : safe_string(req.value("imageUri", "").c_str());
    int quantity = req.value("quantity", 0);

    static const char* sql =
        "INSERT INTO product (name, description, barcode, image_uri, quantity) "
        "VALUES ($1, $2, $3, $4, $5) RETURNING id_product;";

    std::string quantityStr = std::to_string(quantity);
    const char* params[5] = {
        name.c_str(),
        description.c_str(),
        barcode.c_str(),
        imageUri.empty() ? nullptr : imageUri.c_str(),
        quantityStr.c_str()
    };

    PGresult* res = PQexecParams(
        conn_,
        sql,
        5,
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

    static const char* sql_link =
        "INSERT INTO user_product (id_user, id_product) VALUES ($1, $2);";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(product_id).c_str()
    };

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

    if (res) 
    {
        PQclear(res);
    }

    log_file_.log("Product added successfully for profile: {}", profile_id);
}

void SqlCommander::add_warehouses(const std::string& profile_id, const std::string& payload)
{
    auto req = nlohmann::json::parse(payload);

    std::string warehouseName = safe_string(req.value("warehousesName", "").c_str());

    static const char* sql =
        "INSERT INTO warehouses (warehouse_name) "
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

    static const char* sql_link =
        "INSERT INTO user_warehouse (id_user, id_warehouse) VALUES ($1, $2);";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(warehouse_id).c_str()
    };

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

    if (res) 
    {
        PQclear(res);
    }

    log_file_.log("Warehouse added successfully for profile: {}", profile_id);
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

    static const char* delete_link_sql =
        "DELETE FROM user_warehouse WHERE id_user = $1 AND id_warehouse = $2;";

    const char* link_params[2] = {
        profile_id.c_str(),
        std::to_string(warehouse_id).c_str()
    };

    PGresult* res = PQexecParams(conn_, delete_link_sql, 2, nullptr, link_params, nullptr, nullptr, 0);
    if (res) PQclear(res);

    static const char* delete_warehouse_sql =
        "DELETE FROM warehouses WHERE id_warehouse = $1;";

    const char* warehouse_params[1] = { std::to_string(warehouse_id).c_str() };
    res = PQexecParams(conn_, delete_warehouse_sql, 1, nullptr, warehouse_params, nullptr, nullptr, 0);

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
    log_file_.log("delete_warehouses: Warehouse deleted successfully, id: {}", warehouse_id);
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

    PGresult* res = PQexecParams(conn_, sql, 8, nullptr, params, nullptr, nullptr, 0);

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

    PGresult* res = PQexecParams(conn_, sql, 8, nullptr, params, nullptr, nullptr, 0);

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

    static const char* sql =
        "UPDATE product SET name = $1, description = $2, barcode = $3, image_uri = $4, "
        "quantity = $5 WHERE id_product = $6;";

    std::string quantityStr = std::to_string(quantity);
    const char* params[6] = {
        name.c_str(), description.c_str(), barcode.c_str(),
        imageUri.empty() ? nullptr : imageUri.c_str(),
        quantityStr.c_str(), std::to_string(product_id).c_str()
    };

    PGresult* res = PQexecParams(conn_, sql, 6, nullptr, params, nullptr, nullptr, 0);

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

    PGresult* res = PQexecParams(conn_, sql, 2, nullptr, params, nullptr, nullptr, 0);

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
