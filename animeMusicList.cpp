#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <stdlib.h>
#include <api/manager.hpp>

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "mysql_connection.h"

#define HOST "localhost"
#define USER "ID"
#define PASS "PW"
#define DB "anime_music"

using namespace std;
using namespace nlohmann;
using namespace cgicc;

void print_error(string error_code, string error_msg) {
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
string operator+(string a, long long b) {
    return a + to_string(b);
}
string operator+(long long a, string b) {
    return to_string(a) + b;
}
int main(int argc, const char **argv) {
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "anime_music_list");
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    Cgicc form_data;
    unsigned long long page, per_page;
    bool beautify;
    form_iterator data;
    string sort_by;

    data = form_data.getElement("page");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            page = stoull(**data);
        } catch (exception &e) {
            page = 1;
        }
    } else
        page = 1;

    data = form_data.getElement("perPage");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            per_page = stoull(**data);
        } catch (exception &e) {
            per_page = 15;
        }
    } else
        per_page = 15;

    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;
    else
        beautify = false;
    sort_by = string(**form_data.getElement("sort"));
    if (sort_by != "artist" && sort_by != "ja_name" && sort_by != "ko_name" && sort_by != "link") {
        sort_by = "ko_name";
    }
    string url = HOST;
    const string user = USER;
    const string pass = PASS;
    const string database = DB;
    json result;
    long long count;
    result["result"] = json::array();
    try {
        sql::Driver *driver = get_driver_instance();
        sql::Connection *con = driver->connect(url, user, pass);
        con->setSchema(database);
        sql::Statement *stmt = con->createStatement();

        stmt->execute("SELECT COUNT(*) FROM list");
        sql::ResultSet *res;
        res = stmt->getResultSet();
        res->next();
        count = res->getInt64("COUNT(*)");
        result["total"] = count;
    } catch (sql::SQLException &e) {
        print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
    }
    try {
        sql::Driver *driver = get_driver_instance();
        sql::Connection *con = driver->connect(url, user, pass);
        con->setSchema(database);
        sql::Statement *stmt = con->createStatement();

        stmt->execute("SELECT * FROM `list` ORDER BY `" + sort_by + "` ASC LIMIT " + to_string((page - 1) * per_page) + "," + to_string(per_page));
        sql::ResultSet *res;
        do {
            res = stmt->getResultSet();
            result["num"] = res->rowsCount();
            while (res->next()) {
                result["result"].push_back({{"ko_name", res->getString("ko_name")},
                                            {"ja_name", res->getString("ja_name")},
                                            {"link", "https://youtu.be/" + res->getString("link")},
                                            {"artist", res->getString("artist")}});
            }
        } while (stmt->getMoreResults());
        cout << result.dump(beautify ? 4 : -1);
    } catch (sql::SQLException &e) {
        print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
    }
    return EXIT_SUCCESS;
}
