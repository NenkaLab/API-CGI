#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <openssl/hmac.h>
#include <openssl/md5.h>
#include <openssl/x509.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <api/manager.hpp>
#include <syslog.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <filesystem>
#include <nlohmann/json.hpp>
#include <xml2json.hpp>

#include "tinyxml2.cpp"
#define MAX_DATE_LEN 30
using namespace std;
using namespace tinyxml2;
using json = nlohmann::json;
using namespace cgicc;
namespace fs = std::filesystem;
size_t writefunc(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
void print_error(string error_code, string error_msg) {
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
string urlDecode(string SRC) {
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
int main() {
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "pixiv_image");
    bool beautify = false;
    string url = "https://i.pximg.net/", s, ctype;
    Cgicc form_data;
    form_iterator data;
    data = form_data.getElement("url");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = urlDecode(string(**data));
        if (s.starts_with("https://") || s.starts_with("http://"))
            url = s;
        else
            url += s;
    }
    CURL *curl;
    curl = curl_easy_init();
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Referer: https://www.pixiv.net/");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PixivAndroidApp/5.0.219 (Android 10; Redmi Note 7)");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ctype);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    cout << "Content-Type: " << ctype << "\n\n";
    cout << s;
    return 0;
}
//0 1 2 3
//0 0