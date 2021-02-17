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
#include <syslog.h>
#include <tinyxml2.h>
#include <unistd.h>

#include <api/manager.hpp>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <xml2json.hpp>

#include "tinyxml2.cpp"
#include <Magick++.h>
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
string gen_random(const int len) {
    string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    srand((unsigned)time(NULL) * getpid());

    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i)
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];

    return tmp_s;
}
string if_null(char *a, char *b){
    return a == nullptr ? b : a;
}
int main() {
    APIManager::log(if_null(getenv("REMOTE_ADDR"), "localhost"), if_null(getenv("HTTP_USER_AGENT"), "Debug Client"), if_null(getenv("REQUEST_URI"), "N/A"), if_null(getenv("REQUEST_METHOD"), "GET"), "ascii_art");
    string url, s, ctype, mode = "dark";
    Cgicc form_data;
    form_iterator data;
    int width = 50;
    cout << "Content-Type: text/plain\n\n";
    fflush(stdout);
    data = form_data.getElement("url");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = string(**data);
        if (s.starts_with("https://") || s.starts_with("http://"))
            url = s;
        else print_error("S1004", "잘못된 링크형식입니다.");
    }
    else
        print_error("S1003", "링크가 없습니다.");
    data = form_data.getElement("width");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = string(**data);
        try{
            width = stoi(s);
        }catch(exception &e){}
    }
    data = form_data.getElement("mode");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = string(**data);
        if(s == "light" || s == "dark") mode = s;
    }
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Accept-Language: ko-KR,ko;q=0.8,en-US;q=0.5,en;q=0.3");
    hs = curl_slist_append(hs, "Accept-Encoding: gzip, deflate, br");
    hs = curl_slist_append(hs, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8");
    hs = curl_slist_append(hs, "cache-control: no-cache");
    hs = curl_slist_append(hs, "Pragma: no-cache");
    CURL *curl;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:84.0) Gecko/20100101 Firefox/84.0");
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ctype);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    Magick::Image img(Magick::Blob((const void *)s.c_str(), s.length()));
    string path = "/tmp/ascii_tmp_image_" + gen_random(10) + ".jpg";
    img.write(path.c_str());
    system(("jp2a --width=" + to_string(width) + " -i " + path + " --background=" + mode + " --chars=\"     ...''''\\\"\\\"\\\",:;\\**??$$9@@@@#\"").c_str());
    system(("rm " + path).c_str());
    return 0;
}