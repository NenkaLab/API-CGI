#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <openssl/hmac.h>
#include <openssl/x509.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <tinyxml2.h>
#include <unistd.h>
#include <api/manager.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <xml2json.hpp>

#include "tinyxml2.cpp"
using namespace std;
using namespace tinyxml2;
using json = nlohmann::json;
using namespace cgicc;
namespace fs = std::filesystem;
json article_list = json::array();
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
const string ENV[24] = {
    "COMSPEC", "DOCUMENT_ROOT", "GATEWAY_INTERFACE",
    "HTTP_ACCEPT", "HTTP_ACCEPT_ENCODING",
    "HTTP_ACCEPT_LANGUAGE", "HTTP_CONNECTION",
    "HTTP_HOST", "HTTP_USER_AGENT", "PATH",
    "QUERY_STRING", "REMOTE_ADDR", "REMOTE_PORT",
    "REQUEST_METHOD", "REQUEST_URI", "SCRIPT_FILENAME",
    "SCRIPT_NAME", "SERVER_ADDR", "SERVER_ADMIN",
    "SERVER_NAME", "SERVER_PORT", "SERVER_PROTOCOL",
    "SERVER_SIGNATURE", "SERVER_SOFTWARE"};
long long timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}
string base64_encode(unsigned char const *bytes_to_encode, unsigned int in_len) {
    string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];
        while ((i++ < 3))
            ret += '=';
    }
    return ret;
}
string hmac_sha1(string text, string key) {
    unsigned int result_len;
    unsigned char result[EVP_MAX_MD_SIZE];
    HMAC(EVP_sha1(), (unsigned char *)key.c_str(), key.length(), (unsigned char *)text.c_str(), text.length(), result, &result_len);
    string a((char *)result, result_len);
    return a.erase(result_len, a.length() - 1);
}
string url_encode(const string &value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;
    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        escaped << uppercase;
        escaped << '%' << setw(2) << int((unsigned char)c);
        escaped << nouppercase;
    }
    return escaped.str();
}
string key = "Vj4jD4n5lUf9g2I0y7QyilPDPiiusHJb5kTdZDV7b2UUARhPFXPxPdRXtgK3Ej8k";
string get_url(string url) {
    string d;
    long long now = timestamp();
    d = hmac_sha1(url + to_string(now), key);
    d = url_encode(base64_encode((unsigned char *)d.c_str(), d.length()));
    return url + (url.find("?") != std::string::npos ? "&" : "?") + "msgpad=" + to_string(now) + "&md=" + d;
}
size_t writefunc(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
CURL *curl;
void f(json &j) {
    for (auto &[key, value] : j.items()) {
        if (value.is_object() || value.is_array()) {
            try {
                value.at("#text");
                value = value["#text"].get<string>();
            } catch (exception err) {
                f(value);
            }
        }
    }
}
void print_error(string error_code, string error_msg) {
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
int main() {
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "naver_cafe_article_list");
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    Cgicc form_data;
    CURLcode res;
    string page, cafe_id, menu_id, per_page;
    bool beautify;
    form_iterator data;

    data = form_data.getElement("page");
    if (!data->isEmpty() && data != (*form_data).end())
        page = **data;
    else
        page = "1";

    data = form_data.getElement("perPage");
    if (!data->isEmpty() && data != (*form_data).end())
        per_page = **data;
    else
        per_page = "15";

    data = form_data.getElement("cafeId");
    if (!data->isEmpty() && data != (*form_data).end())
        cafe_id = **data;
    else
        print_error("1", "cafeId가 누락되었습니다.");

    data = form_data.getElement("menuId");
    if (!data->isEmpty() && data != (*form_data).end())
        menu_id = **data;
    else
        menu_id = "";

    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;
    else
        beautify = false;
    curl = curl_easy_init();
    string s, a = get_url("https://apis.naver.com/cafemobileapps/cafe/ArticleList.xml?search.clubid=" + url_encode(cafe_id) + "&search.menuid=" + url_encode(menu_id) + "&search.perPage=" + url_encode(per_page) + "&search.page=" + url_encode(page) + "&search.queryType=lastArticle&moreManageMenus=false");
    curl_easy_setopt(curl, CURLOPT_URL, a.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    try {
        XMLDocument doc;
        doc.Parse(s.c_str(), s.length());
        if (doc.FirstChildElement("result")) {
            auto er = doc.FirstChildElement("result");
            print_error(string(er->FirstChildElement("error_code")->GetText()), "내부오류: " + string(er->FirstChildElement("message")->GetText()) + "\\nhunhee@kakao.com으로 문의해 주세요.");
        }
        if (doc.FirstChildElement("message")->FirstChildElement("error")) {
            auto a = doc.FirstChildElement("message")->FirstChildElement("error");
            print_error(string(a->FirstChildElement("code")->GetText()), string(a->FirstChildElement("msg")->GetText()));
        }
        auto *textNode = doc.FirstChildElement("message")
                             ->FirstChildElement("result")
                             ->FirstChildElement("articleList")
                             ->FirstChildElement("articles");
        int64_t id;
        XMLPrinter printer;
        textNode->Accept(&printer);
        json j = json::parse(xml2json(printer.CStr()));
        json im = json::array();
        if (!j["articles"]["article"].is_array() && j["articles"]["article"].is_object())
            im.push_back(j["articles"]["article"]);
        else if (j["articles"]["article"].is_array())
            im = j["articles"]["article"];
        j["result"]["article"] = im;
        j.erase("articles");
        f(j);
        if (beautify)
            cout << j.dump(4);
        else
            cout << j.dump();
    } catch (exception &e) {
        string ex(e.what());
        replace(ex.begin(), ex.end(), '\n', '\t');
        print_error("0", "내부 오류\\n" + ex + "\\nhunhee@kakao.com으로 문의해 주세요.");
    }
    return 0;
}