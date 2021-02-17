#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include <api/manager.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#include "mysql_connection.h"

#define HOST "localhost"
#define USER "ID"
#define PASS "PW"
#define DB "short_url"

using namespace std;
using namespace nlohmann;
using namespace cgicc;

void print_error(string error_code, string error_msg)
{
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}",
        error_code.c_str(), error_msg.c_str());
    exit(0);
}
string operator+(string a, long long b) { return a + to_string(b); }
string operator+(long long a, string b) { return to_string(a) + b; }
string replace_all(string str, string from, string to)
{
    size_t pos = 0, offset = 0;
    while ((pos = str.find(from, offset)) != string::npos) {
        str.replace(str.begin() + pos, str.begin() + pos + from.size(), to);
        offset = pos + to.size();
    }
    return str;
}
string escape_string(string str)
{
    return replace_all(replace_all(str, "\"", "\\\""), "\'", "\\'");
}
string url_decode(string SRC)
{
    string ret;
    char ch;
    int i, ii;
    for (i = 0; i < SRC.length(); i++) {
        if (int(SRC[i]) == 37) {
            sscanf(SRC.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        } else {
            ret += SRC[i];
        }
    }
    return (ret);
}
string gen_random(const int len)
{
    string tmp_s;
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    srand((unsigned)time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}
int main(int argc, const char** argv)
{
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"),
        getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "short_url");
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    Cgicc form_data;
    form_iterator data;
    string url;

    url = url_decode(string(**form_data.getElement("url")));
    if (url.length() >= 10000 || url.length() <= 5) {
        print_error("403", "url의 길이는 5에서 10000사이여야 합니다.");
    }
    if (!url.starts_with("https://") && !url.starts_with("http://")) {
        print_error("403", "url은 http://또는 https://로 시작하여야 합니다.");
    }
    const string host = HOST;
    const string user = USER;
    const string pass = PASS;
    const string database = DB;
    json result;
    long long count;
    url = escape_string(url);
    try {
        sql::Driver* driver = get_driver_instance();
        sql::Connection* con = driver->connect(host, user, pass);
        con->setSchema(database);
        sql::Statement* stmt = con->createStatement();

        stmt->execute("SELECT * FROM list WHERE origin_url='" + url + "'");
        sql::ResultSet* res;
        res = stmt->getResultSet();
        if (res->next()) {
            result["result"] = "http://poox.ml/u/" + res->getString("short");
            cout << result.dump();
            exit(EXIT_SUCCESS);
        }
        con->close();
    } catch (sql::SQLException& e) {
        print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
    }
    string short_link;
    int i = 1;
    while (true) {
        try {
            short_link = gen_random(i);
            sql::Driver* driver = get_driver_instance();
            sql::Connection* con = driver->connect(host, user, pass);
            con->setSchema(database);
            sql::Statement* stmt = con->createStatement();
            stmt->execute("SELECT * FROM list WHERE short='" + short_link + "'");
            if (!stmt->getResultSet()->rowsCount()) {
                break;
                con->close();
            }
            con->close();
            i++;
        } catch (sql::SQLException& e) {
            print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
        }
    }
    try {
        sql::Driver* driver = get_driver_instance();
        sql::Connection* con = driver->connect(host, user, pass);
        con->setSchema(database);
        sql::Statement* stmt = con->createStatement();

        stmt->execute(
            "INSERT INTO `list` (`id`, `short`, `origin_url`, `created_at`) VALUES "
            "(NULL, '"
            + short_link + "', '" + url + "', current_timestamp());");
        sql::ResultSet* res;
        res = stmt->getResultSet();
        con->close();
    } catch (sql::SQLException& e) {
        print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
    }
    result["result"] = "http://poox.ml/u/" + short_link;
    cout << result.dump();
    return EXIT_SUCCESS;
}
