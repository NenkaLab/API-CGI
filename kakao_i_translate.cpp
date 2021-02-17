#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
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
string url_decode(string SRC) {
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
void print_error(string error_code, string error_msg) {
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
string replace_all(string str, const string &from, const string &to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
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
// const string API_HEADER = "Authorization: KakaoAK a0e3431efb424074473d8e7fbc366076";
// string detect_lang(string a) {
//     curl_global_init(CURL_GLOBAL_ALL);
//     string url = "https://dapi.kakao.com/v3/translation/language/detect?query=" + url_encode(a), s;
//     CURL *curl;
//     curl = curl_easy_init();
//     struct curl_slist *hs = NULL;
//     hs = curl_slist_append(hs, API_HEADER.c_str());
//     curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//     curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36");
//     curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
//     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
//     curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
//     curl_easy_perform(curl);
//     curl_easy_cleanup(curl);
//     curl_global_cleanup();
//     try {
//         return json::parse(s)["language_info"][0]["code"].get<string>();
//     } catch (exception &e) {
//         print_error("500", "kakao i에서 예기치 못한 응답을 반환하였습니다." + string(e.what()));
//     }
//     return "{\"error\":\"unhandled error!\"}";
// }
int main() {
    bool beautify = false, honorific = false;
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "kakao_i_translate");

    string url = "", text, s, src, tar;
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    Cgicc form_data;
    form_iterator data;
    data = form_data.getElement("source");
    if (!data->isEmpty() && data != (*form_data).end())
        honorific = **data == "true" ? true : false;
    data = form_data.getElement("text");
    if (!data->isEmpty() && data != (*form_data).end()) {
        if (url_decode(string(**data)).length() >= 5000) {
            print_error("100", "번역할 문장의 길이는 5000자를 넘길 수 없습니다.");
        }
        url += "q=" + replace_all(string(**data), "%20", "+");
        text = string(**data);
    } else {
        print_error("001", "text(이)가 없습니다.");
    }
    data = form_data.getElement("source");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = string(**data);
        if (s == "unk")
            s = "auto";
        else if (s == "ko")
            s = "kr";
        else if (s == "ja")
            s = "jp";
        else if (s == "zh-CN")
            s = "cn";
        else if (s == "zh-TW")
            s = "cn";
        if (s == "kr" && honorific) {
            s = "ku";
        }
        // if (s == "auto") {
        //     s = detect_lang(text);
        // }
        src = s;
        url += "&queryLanguage=" + s;
    } else {
        print_error("001", "source(이)가 없습니다.");
    }
    data = form_data.getElement("target");
    if (!data->isEmpty() && data != (*form_data).end()) {
        string s = string(**data);
        if (s == "ko")
            s = "kr";
        else if (s == "ja")
            s = "jp";
        else if (s == "zh-CN")
            s = "cn";
        else if (s == "zh-TW")
            s = "cn";
        if (s == "kr" && honorific) {
            s = "ku";
        }
        tar = s;
        url += "&resultLanguage=" + s;
    } else {
        print_error("001", "target(이)가 없습니다.");
    }
    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;
    data = form_data.getElement("honorific");
    if(tar == src){
        json re;
        re["result"] = {{"lang", {{"source", src}, {"target", tar}}}};
        re["result"]["input"] = text;
        re["result"]["output"] = text;
        cout << re.dump(beautify ? 4 : -1);
        exit(EXIT_SUCCESS);
    }
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "content-type: application/x-www-form-urlencoded; charset=UTF-8");
    hs = curl_slist_append(hs, "Origin: https://translate.kakao.com");
    hs = curl_slist_append(hs, "Referer: https://translate.kakao.com/");
    hs = curl_slist_append(hs, "user-agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_URL, "https://translate.kakao.com/translator/translate.json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    try {
        json org = json::parse(s), re;
        // cout << org.dump(4) << endl;
        fflush(stdout);
        string s;
        re["result"] = {{"lang", {{"source", src}, {"target", tar}}}};
        re["result"]["input"] = text;
        re["result"]["output"] = "";
        for (auto &e : org["result"]["output"].items()) {
            for (auto &e1 : e.value().items())
                re["result"]["output"] = re["result"]["output"].get<string>() + e1.value().get<string>() + "\n";
            re["result"]["output"] = re["result"]["output"].get<string>() + "\n";
        }
        s = re["result"]["output"].get<string>();
        s.pop_back();
        re["result"]["output"] = s;
        cout << re.dump(beautify ? 4 : -1);
    } catch (exception &e) {
        print_error("500", "kakao i에서 잘못된 응답을 반환하였습니다." + string(e.what()));
    }
    return 0;
}