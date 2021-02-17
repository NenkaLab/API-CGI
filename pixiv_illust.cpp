///작동안함!!
#include <api/manager.hpp>
#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/x509.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <tinyxml2.h>
#include <unistd.h>
#include <xml2json.hpp>

#include "tinyxml2.cpp"
#define MAX_DATE_LEN 30
using namespace std;
using namespace tinyxml2;
using json = nlohmann::json;
using namespace cgicc;
namespace fs = std::filesystem;
json article_list = json::array();
const string HASH_SECRET = "28c1fdd170a5204386cb1313c7077b34f83e4aaf4aa829ce78c231e05b0bae2c";
const string CLIENT_SECRET = "lsACyCD94FhDUtGTXi3QzcFE2uU1hqtDaKeqrdwj";
const string CLIENT_ID = "MOBrBDS8blbauoSck0ZfDbtuzpyT";
string access_token;
long long timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}
string toHex(string input)
{
    static const char hex_digits[] = "0123456789abcdef";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input) {
        output.push_back(hex_digits[c >> 4]);
        output.push_back(hex_digits[c & 15]);
    }
    return output;
}
string md5(string text)
{
    unsigned char md[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)text.c_str(), text.length(), md);
    string a((char*)md, MD5_DIGEST_LENGTH);
    return a;
}
string get_client_hash(string time)
{
    string md5_enc = md5(time + HASH_SECRET);
    return toHex(md5_enc);
}
size_t writefunc(char* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
json pixiv_login(string username, string password)
{
    CURL* curl;
    curl = curl_easy_init();
    const string auth_url = "https://oauth.secure.pixiv.net/auth/token";
    time_t timer;
    char buffer[MAX_DATE_LEN];
    struct tm* tm_info;
    bool is_token_exist = false;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(buffer, MAX_DATE_LEN, "%Y-%m-%dT%H:%M:%S%z", tm_info);
    memmove(buffer + 22, buffer + 21, MAX_DATE_LEN - 22); //0900 to 09:00
    buffer[22] = ':';
    string data = "", now_time(buffer), s;
    data += "get_secure_url=1";
    data += "&client_id=" + CLIENT_ID;
    data += "&client_secret=" + CLIENT_SECRET;
    FILE* fp = fopen("/tmp/pixiv_refresh_token", "r");
    if (fp != NULL) {
        is_token_exist = true;
        char token[1000];
        fscanf(fp, "%s", token);
        fclose(fp);
        data += "&grant_type=refresh_token";
        data += "&refresh_token=" + string(token);
    } else {
        data += "&grant_type=password";
        data += "&username=" + username;
        data += "&password=" + password;
    }
    struct curl_slist* hs = NULL;
    hs = curl_slist_append(hs, ("x-client-time: " + now_time).c_str());
    hs = curl_slist_append(hs, ("x-client-hash: " + get_client_hash(now_time)).c_str());
    hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
    hs = curl_slist_append(hs, "accept-language: ko_KR");
    hs = curl_slist_append(hs, "app-os: android");
    hs = curl_slist_append(hs, "app-os-version: 10");
    hs = curl_slist_append(hs, "app-version: 5.0.219");
    curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PixivAndroidApp/5.0.219 (Android 10; Redmi Note 7)");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    json result = json::parse(s);
    if (!is_token_exist) {
        fp = fopen("/tmp/pixiv_refresh_token", "w");
        assert(fp != NULL);
        fprintf(fp, "%s", result["refresh_token"].get<string>().c_str());
        fclose(fp);
    }
    return result;
}
void print_error(string error_code, string error_msg)
{
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
json illust_detail(string access_token, string id)
{
    CURL* curl;
    curl = curl_easy_init();
    const string auth_url = "https://app-api.pixiv.net/v1/illust/detail?filter=for_android&illust_id=" + id;
    time_t timer;
    char buffer[MAX_DATE_LEN];
    struct tm* tm_info;
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(buffer, MAX_DATE_LEN, "%Y-%m-%dT%H:%M:%S%z", tm_info);
    memmove(buffer + 22, buffer + 21, MAX_DATE_LEN - 22); //0900 to 09:00
    buffer[22] = ':';
    string now_time(buffer), s;
    struct curl_slist* hs = NULL;
    hs = curl_slist_append(hs, ("x-client-time: " + now_time).c_str());
    hs = curl_slist_append(hs, ("x-client-hash: " + get_client_hash(now_time)).c_str());
    hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
    hs = curl_slist_append(hs, "accept-language: ko_KR");
    hs = curl_slist_append(hs, "app-os: android");
    hs = curl_slist_append(hs, "app-os-version: 10");
    hs = curl_slist_append(hs, "app-version: 5.0.219");
    hs = curl_slist_append(hs, ("authorization: Bearer " + access_token).c_str());
    curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PixivAndroidApp/5.0.219 (Android 10; Redmi Note 7)");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    json org = json::parse(s), result;
    try {
        print_error("002", org.at("error").at("user_message").get<string>());
    } catch (exception& e) {
    }
    // result = org;
    result["illust"]["image_urls"] = json::array();
    if (org["illust"]["page_count"].get<long long>() == 1) {
        result["illust"]["image_urls"].push_back(org["illust"]["image_urls"]);
        result["illust"]["image_urls"][0]["original"] = org["illust"]["meta_single_page"]["original_image_url"];
    } else {
        for (auto& e : org["illust"]["meta_pages"].items()) {
            result["illust"]["image_urls"].push_back(e.value()["image_urls"]);
        }
    }
    result["illust"]["caption"] = org["illust"]["caption"];
    result["illust"]["tags"] = org["illust"]["tags"];
    result["illust"]["user"] = org["illust"]["user"];
    result["illust"]["user"].erase("is_followed");
    result["illust"]["create_date"] = org["illust"]["create_date"];
    result["illust"]["height"] = org["illust"]["height"];
    result["illust"]["width"] = org["illust"]["width"];
    result["illust"]["sanity_level"] = org["illust"]["sanity_level"];
    result["illust"]["page_count"] = org["illust"]["page_count"];
    result["illust"]["id"] = org["illust"]["id"];
    result["illust"]["series"] = org["illust"]["series"];
    result["illust"]["title"] = org["illust"]["title"];
    result["illust"]["tools"] = org["illust"]["tools"];
    result["illust"]["total_bookmarks"] = org["illust"]["total_bookmarks"];
    result["illust"]["total_comments"] = org["illust"]["total_comments"];
    result["illust"]["total_view"] = org["illust"]["total_view"];
    result["illust"]["type"] = org["illust"]["type"];
    return result;
}
int main()
{
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "pixiv_illust");
    bool beautify = false;
    string id;
    Cgicc form_data;
    form_iterator data;
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    data = form_data.getElement("id");
    if (!data->isEmpty() && data != (*form_data).end())
        id = string(**data);
    else
        print_error("403", "id(이)가 빠졌습니다.");

    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;
    access_token = pixiv_login("ID", "PW")["access_token"].get<string>();
    try {
        cout << illust_detail(access_token, id).dump(beautify ? 4 : -1);
    } catch (exception& e) {
        print_error("500", (string) "pixiv에서 예기치 못한 오류\\n" + e.what());
    }
    return 0;
}
//0 1 2 3
//0 0
