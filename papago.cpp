#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <openssl/hmac.h>
#include <api/manager.hpp>
#include <openssl/x509.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <tinyxml2.h>
#include <unistd.h>

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
string gen_uuid() {
    static random_device dev;
    static mt19937 rng(dev());

    uniform_int_distribution<int> dist(0, 15);

    const char *v = "0123456789abcdef";
    const bool dash[] = {0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0};

    string res;
    for (int i = 0; i < 16; i++) {
        if (dash[i]) res += "-";
        res += v[dist(rng)];
        res += v[dist(rng)];
    }
    res[14] = '4';
    res[19] = 'y';
    return res;
}
template <typename T>
typename std::enable_if<std::is_integral<T>::value, std::string>::type toHex(const T &value) {
    std::stringstream convertingStream;
    convertingStream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << value;
    return convertingStream.str();
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
string hmac_md5(string text, string key) {
    unsigned int result_len;
    unsigned char result[EVP_MAX_MD_SIZE];
    HMAC(EVP_md5(), (unsigned char *)key.c_str(), key.length(), (unsigned char *)text.c_str(), text.length(), result, &result_len);
    string a((char *)result, result_len);
    return a;
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
std::map<string, string> get_uuid(string url, long long ts) {
    string uuid = gen_uuid();
    std::map<string, string> m;
    string s = hmac_md5(uuid + "\n" + url + "\n" + to_string(ts), "v1.5.1_4dfe1d83c2");
    s = base64_encode((unsigned char *)s.c_str(), s.length());
    m["authorization"] = "PPG " + uuid + ":" + s;
    m["uuid"] = uuid;
    return m;
}
size_t writefunc(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
CURL *curl;
void print_error(string error_code, string error_msg) {
    printf("{\"error_code\": \"%s\", \"error_message\": \"%s\"}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
json translate(string source, string target, string text, bool honorific, string locale, bool dict, int dict_display, bool instance, bool paging, unsigned long long page, unsigned long long per_page) {
    long long ts = timestamp();
    curl = curl_easy_init();
    string s, a = "https://papago.naver.com/apis/n2mt/translate";
    std::map<string, string> m = get_uuid(a, ts);
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
    hs = curl_slist_append(hs, "accept-language: ko");
    hs = curl_slist_append(hs, "accept: application/json");
    hs = curl_slist_append(hs, "x-apigw-partnerid: papago");
    hs = curl_slist_append(hs, "device-type: pc");
    hs = curl_slist_append(hs, "origin: https://papago.naver.com");
    hs = curl_slist_append(hs, "pragma: no-cache");
    hs = curl_slist_append(hs, "referer: https://papago.naver.com/");
    hs = curl_slist_append(hs, "sec-fetch-dest: empty");
    hs = curl_slist_append(hs, "sec-fetch-mode: cors");
    hs = curl_slist_append(hs, "sec-fetch-site: same-origin");
    hs = curl_slist_append(hs, ("authorization: " + m["authorization"]).c_str());
    hs = curl_slist_append(hs, ("timestamp: " + to_string(ts)).c_str());
    string data;
    data = "deviceId=" + url_encode(m["uuid"]);
    data += "&locale=" + locale;
    data += (string) "&dict=" + (dict ? "true" : "false");
    data += "&dictDisplay=" + to_string(dict_display);
    data += (string) "&honorific=" + (honorific ? "true" : "false");
    data += (string) "&instance=" + (instance ? "true" : "false");
    data += (string) "&paging=" + (paging ? "true" : "false");
    data += "&page=" + to_string(page);
    data += "&perPage=" + to_string(per_page);
    data += "&source=" + url_encode(source);
    data += "&target=" + url_encode(target);
    data += "&text=" + text;
    curl_easy_setopt(curl, CURLOPT_URL, a.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return json::parse(s);
}
string lanuguage_auto_detect(string text) {
    long long ts = timestamp();
    curl = curl_easy_init();
    string s, a = "https://papago.naver.com/apis/langs/dect";
    std::map<string, string> m = get_uuid(a, ts);
    struct curl_slist *hs = NULL;
    hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded; charset=UTF-8");
    hs = curl_slist_append(hs, "accept-language: ko");
    hs = curl_slist_append(hs, "accept: application/json");
    hs = curl_slist_append(hs, "x-apigw-partnerid: papago");
    hs = curl_slist_append(hs, "device-type: pc");
    hs = curl_slist_append(hs, "origin: https://papago.naver.com");
    hs = curl_slist_append(hs, "pragma: no-cache");
    hs = curl_slist_append(hs, "referer: https://papago.naver.com/");
    hs = curl_slist_append(hs, "sec-fetch-dest: empty");
    hs = curl_slist_append(hs, "sec-fetch-mode: cors");
    hs = curl_slist_append(hs, "sec-fetch-site: same-origin");
    hs = curl_slist_append(hs, ("authorization: " + m["authorization"]).c_str());
    hs = curl_slist_append(hs, ("timestamp: " + to_string(ts)).c_str());
    string data;
    data = "query=" + text;
    curl_easy_setopt(curl, CURLOPT_URL, a.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/85.0.4183.121 Safari/537.36");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return json::parse(s)["langCode"].get<string>();
}
int main() {
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "papago_translate");
    cout << "Content-Type: application/json; charset=utf-8\n\n";
    Cgicc form_data;
    unsigned long long page = 1, per_page = 30, dict_display = 30;
    bool beautify = false, dict = true, honorific = false, instance = true, paging = false;
    form_iterator data;
    string target, source, text, locale = "ko";  //string locale, bool dict, int dictDisplay, bool instance, bool paging)

    data = form_data.getElement("source");
    if (!data->isEmpty() && data != (*form_data).end())
        source = url_encode(string(**data));
    else
        print_error("403", "source(이)가 빠졌습니다.");

    data = form_data.getElement("target");
    if (!data->isEmpty() && data != (*form_data).end())
        target = url_encode(string(**data));
    else
        print_error("403", "target(이)가 빠졌습니다.");

    data = form_data.getElement("text");
    if (!data->isEmpty() && data != (*form_data).end())
        text = url_encode(string(**data));
    else
        print_error("403", "text(이)가 빠졌습니다.");

    data = form_data.getElement("honorific");
    if (!data->isEmpty() && data != (*form_data).end())
        honorific = **data == "true" ? true : false;

    data = form_data.getElement("locale");
    if (!data->isEmpty() && data != (*form_data).end())
        locale = url_encode(string(**data));

    data = form_data.getElement("page");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            page = stoull(**data);
        } catch (exception &e) {
        }
    }

    data = form_data.getElement("perPage");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            per_page = stoull(**data);
        } catch (exception &e) {
        }
    }

    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;

    data = form_data.getElement("dict");
    if (!data->isEmpty() && data != (*form_data).end())
        dict = **data == "true" ? true : false;

    data = form_data.getElement("instance");
    if (!data->isEmpty() && data != (*form_data).end())
        instance = **data == "true" ? true : false;

    data = form_data.getElement("paging");
    if (!data->isEmpty() && data != (*form_data).end())
        paging = **data == "true" ? true : false;

    data = form_data.getElement("dictDisplay");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            dict_display = stoull(**data);
        } catch (exception &e) {
        }
    }

    if (source == "unk") {
        source = lanuguage_auto_detect(text);
        if (source == "unk") source = "ko";
    }

    try {
        cout << translate(source, target, text, honorific, locale, dict, dict_display, instance, paging, page, per_page).dump(beautify ? 4 : -1);
    } catch (exception &e) {
        string ex(e.what());
        replace(ex.begin(), ex.end(), '\n', '\t');
        print_error("0", "내부 오류\\n" + ex + "\\nhunhee@kakao.com으로 문의해 주세요.");
    }
    return 0;
}