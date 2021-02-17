#include <bits/stdc++.h>
#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <openssl/rsa.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <api/manager.hpp>
#include <time.h>
#include <unistd.h>

#include <nlohmann/json.hpp>
using namespace std;
using namespace cgicc;
using json = nlohmann::json;
size_t writefunc(char *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}
string get_enc_data() {
    unsigned char c[] =
        "\xdf\xbc\x1f\x3f\x4c\x10\xe1\x7e\x01\x12\xd7\x2e\x78\x91\x6d\xa5\x06\xed\xd5\x7d\xa0\x6e\xac\x6a\xe4\xf0\x0d\xd3\x01\x06\x71\x78\x05\x7b\xaa\x9b\xa9\x4e\xf6\xe6\x65\xbf\xb2\x9c\xee\x56\x7d\xe4\x08\x12\x49\xc0\xbe\x37\x6f\x98\x11\x38\x3c\xe6\xd1\x2b\xad\x74\x4a\x2f\x12\xfc\x16\x18\x9c\x3d\x6e\xc0\x41\x22\x2b\x45\x95\x41\x84\x16\x5f\x37\xd9\x8d\x18\x8e\xd5\xad\x15\x8f\xf8\xb5\x00\x4e\x8e\x71\x7f\x71\x4f\xc9\x62\xab\x7e\xb0\x2d\x58\x48\x19\x60\xd4\xd6\x2f\x09\xc0\xb6\x42\xe4\x96\xec\x70\x3e\xca\x1c"
        "e7K";
    unsigned char *from = (unsigned char *)malloc(100 * sizeof(unsigned char));
    unsigned char a[4] = "\x01\0\x01";
    from = (unsigned char *)malloc(sizeof(unsigned char) * 31);
    time_t now_time = time(NULL);
    strftime((char *)from, 100, "ALSONG_ANDROID_%Y%m%d_%H%M%S", gmtime(&now_time));
    RSA *enc = RSA_new();
    RSA_set0_key(enc, BN_bin2bn(c, 0x80, (BIGNUM *)0x0), BN_bin2bn(a, 3, (BIGNUM *)0x0), (BIGNUM *)0x0);
    RSA_set0_factors(enc, (BIGNUM *)0x0, (BIGNUM *)0x0);
    RSA_set0_crt_params(enc, (BIGNUM *)0x0, (BIGNUM *)0x0, (BIGNUM *)0x0);
    size_t size = RSA_size(enc), flen = strlen((char *)from);
    unsigned char *to = (unsigned char *)malloc(size * sizeof(unsigned char));
    memset(to, 0, size);
    RSA_public_encrypt(flen, from, to, enc, 1);
    string ret;
    for (int i = 0; i < size; i++) {
        char a[10] = {};
        sprintf(a, "%02hX", to[i]);
        ret.append(a);
    }
    free(from);
    RSA_free(enc);
    CRYPTO_cleanup_all_ex_data();
    return ret;
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
json get_lyric(long long id) {
    CURL *curl;
    curl = curl_easy_init();
    string data, s;
    struct curl_slist *hs = NULL;
    data = "info_id=" + to_string(id) + "&encData=" + url_encode(get_enc_data());
    hs = curl_slist_append(hs, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8");
    curl_easy_setopt(curl, CURLOPT_URL, "https://lyric.altools.com/v1/info");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Dalvik/2.1.0 (Linux; U; Android 10; Redmi Note 7 Build/QQ3A.200805.001)");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    json obj = json::parse(s);
    obj["id"] = obj["info_id"];
    obj.erase("info_id");
    string str = obj["lyric"].get<string>();
    size_t pos = 0, bpos = 0;
    json lyric_list;
    while ((pos = str.find("<br>", bpos + 1)) != string::npos) {
        string sub = str.substr(bpos, pos - bpos), play_time;
        size_t start = sub.find("["), end = sub.find("]");
        play_time = sub.substr(start + 1, end - start - 1);
        lyric_list[play_time].push_back(sub.substr(end + 1));
        bpos = pos + 4;
    }
    obj["lyric"] = lyric_list;
    return obj;
}
void print_error(string error_code, string error_msg) {
    printf("{\"error\":{\"code\": \"%s\", \"message\": \"%s\"}}", error_code.c_str(), error_msg.c_str());
    exit(0);
}
void detail() {
    bool beautify = false;
    long long id;
    Cgicc form_data;
    form_iterator data;
    curl_global_init(CURL_GLOBAL_ALL);
    setvbuf(stdout, NULL, _IOFBF, 0);
    printf("Content-Type: application/json; charset=utf-8\n\n");
    data = form_data.getElement("id");
    if (!data->isEmpty() && data != (*form_data).end()) {
        try {
            id = stoll(string(**data));
        } catch (exception &e) {
            print_error("404", "id(이)가 잘못되었습니다.");
        }
    } else
        print_error("403", "id(이)가 없습니다.");
    data = form_data.getElement("beautify");
    if (!data->isEmpty() && data != (*form_data).end())
        beautify = **data == "true" ? true : false;
    cout << get_lyric(id).dump(beautify ? 4 : -1);
    curl_global_cleanup();
}
int main() {
    APIManager::log(getenv("REMOTE_ADDR"), getenv("HTTP_USER_AGENT"), getenv("REQUEST_URI"), getenv("REQUEST_METHOD"), "alsong_detail");
    detail();
}