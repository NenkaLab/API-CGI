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

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

#include "mysql_connection.h"

#define HOST "localhost"
#define USER "ID"
#define PASS "PW"
#define DB "short_url"

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
string replace_all(string str, string from, string to) {
    size_t pos = 0, offset = 0;
    while ((pos = str.find(from, offset)) != string::npos) {
        str.replace(str.begin() + pos, str.begin() + pos + from.size(), to);
        offset = pos + to.size();
    }
    return str;
}
string unescape_string(string str) {
    return replace_all(replace_all(str, "\\\"", "\""), "\\'", "'");
}
string get_link(string uri){
    size_t i = 0, len = uri.length();
    string re;
    if((i = uri.find_last_of('/')) == string::npos) return "";
    for(i++; i < len; i++){
        re.push_back(uri[i]);
    }
    return re;
}
int main(int argc, const char **argv) {
    cout << "Content-Type: text/html; charset=utf-8\n";
    Cgicc form_data;
    form_iterator data;
    string url = get_link(getenv("REQUEST_URI"));
    if (url.find('\'') != string::npos || url.find('"') != string::npos) {
        printf("Status: 404 Not Found\n\n죄송합니다. 요청하신 페이지가 유효하지 않습니다 :(\n");
        exit(EXIT_SUCCESS);
    }
    const string host = HOST;
    const string user = USER;
    const string pass = PASS;
    const string database = DB;
    string result;
    long long count;
    try {
        sql::Driver *driver = get_driver_instance();
        sql::Connection *con = driver->connect(host, user, pass);
        con->setSchema(database);
        sql::Statement *stmt = con->createStatement();

        stmt->execute("SELECT * FROM list WHERE short='" + url + "'");
        sql::ResultSet *res;
        res = stmt->getResultSet();
        if (res->next()) {
            result = res->getString("origin_url");
        }
        else{
            printf("Status: 404 Not Found\n\n죄송합니다. 요청하신 페이지가 유효하지 않습니다 :(\n");
            exit(EXIT_SUCCESS);
        }
        con->close();
    } catch (sql::SQLException &e) {
        print_error("0", (string) "내부오류\\nSQL예외" + __FILE__ + "(" + __FUNCTION__ + ")" + __LINE__ + "줄\\n상세: " + e.what() + " (SQL 오류 코드: " + e.getErrorCode() + ", SQL상태: " + e.getSQLState() + " )");
    }
    printf("Status: 301 Moved Permanently\nLocation: %s\n\n", result.c_str());
    return EXIT_SUCCESS;
}
