#include <iostream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <ctime>
#include <poll.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "kalshlib.h"

ConnectionManager::ConnectionManager(){
    //Set public key and stage the private key
    ws_sub_id = 0;
    std::string market = "KXSOL15M";
    updateMarketTicker(ticker, market);
    publicKey = "e86478f2-dd6c-4ae9-9190-064f670aad90";
    FILE* fp = fopen("src/kalshlib/config.txt", "r");
    pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    ctx =  EVP_PKEY_CTX_new(pkey, NULL);
    baseUrl = "https://api.elections.kalshi.com";//"https://demo-api.kalshi.co";
}

std::string ConnectionManager::currentTimeMs(){
    const auto now = std::chrono::system_clock::now();
    std::time_t nowT = std::chrono::system_clock::to_time_t(now);
    long long nowMs = long(nowT * 1000);
    return std::to_string(nowMs);
}

std::string ConnectionManager::generateMsg(std::string &time, std::string &method, std::string &path){
    return time + method + path;
}

std::string ConnectionManager::signMsg(std::string &text, EVP_PKEY* pkey){
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX* pkey_ctx;

    EVP_DigestSignInit(ctx, &pkey_ctx, EVP_sha256(), NULL, pkey);


    EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, RSA_PKCS1_PSS_PADDING);
    EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, EVP_sha256());
    EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, RSA_PSS_SALTLEN_DIGEST);


    EVP_DigestSignUpdate(ctx, text.c_str(), text.size());

    size_t sig_len;
    EVP_DigestSignFinal(ctx, NULL, &sig_len);

    std::vector<unsigned char> sig(sig_len);
    EVP_DigestSignFinal(ctx, sig.data(), &sig_len);

    EVP_MD_CTX_free(ctx);

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); 
    BIO_write(b64, sig.data(), sig_len);
    BIO_flush(b64);

    BUF_MEM* buf;
    BIO_get_mem_ptr(mem, &buf);

    std::string result(buf->data, buf->length);
    BIO_free_all(b64);

    return result;
}
struct ResponseData {
    char *data;
    size_t size;
};

static size_t write_callback(char *payload, size_t size, size_t nmemb, void *userdata){
    size_t realSize = nmemb * size;
    ResponseData *response = (ResponseData *)userdata;

    char *ptr = (char *)realloc(response->data, response->size + realSize + 1);
    if(!ptr){
        std::cout << "Error reallocing memory!";
    }
    response->data = ptr;
    memcpy(&(response->data[response->size]), payload, realSize);
    response->size = response->size + realSize;
    response->data[response->size] = '\0';
    return realSize;
}


void parseJSON(char *input, const char* key, char delim, char *c_out, int *d_out){
    char *start = strstr(input, key);
    if (!start) {
        std::cout << "Error in parsing the JSON, possible bad request?" << std::endl;
        return;
    }
    start += strlen(key) + 2; // moves past ": at the end of the key
    char *end = strchr(start, delim);
    if (!end) {
        std::cout << "Error in parsing the JSON, possible bad request?" << std::endl;
        return;
    }

    if (d_out == nullptr){
        size_t len = end - start;
        memcpy(c_out, start, len);
        c_out += len - 1;
        *c_out = '\0';
    } 
    else {
        char *decimal = strchr(start, '.');
        if (decimal != nullptr){
            std::from_chars(decimal + 1, end, *d_out);
        }
        else {
            std::from_chars(start, end, *d_out);
        }
    }
}

void ConnectionManager::doAuth(std::string method, std::string path, curl_slist *&list){
    std::string timeStamp = currentTimeMs();
    std::string msg = generateMsg(timeStamp, method, path);
    std::string signature = signMsg(msg, pkey);
    std::string h1 = "KALSHI-ACCESS-KEY: ";
    h1 = h1 + publicKey;
    std::string h2 = "KALSHI-ACCESS-SIGNATURE: ";
    h2 = h2 + signature;
    std::string h3 = "KALSHI-ACCESS-TIMESTAMP: ";
    h3 = h3 + timeStamp;
    list = curl_slist_append(list, h1.c_str());
    list = curl_slist_append(list, h2.c_str());
    list = curl_slist_append(list, h3.c_str());
}

int ConnectionManager::getBalance(){
    std::string method = "GET";
    std::string path = "/trade-api/v2/portfolio/balance";
    CURL* curl = curl_easy_init();
    curl_slist *list = NULL;
    doAuth(method, path, list);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, (baseUrl + path).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    ResponseData response;
    response.data = (char *)malloc(1);
    response.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    CURLcode mresult = curl_easy_perform(curl);
    if(mresult){
        std::cout << "Error creating hhtp request";
    }
    const char* key = "balance";
    int balance = 0;
    char delim = ',';
    parseJSON(response.data, key, delim, nullptr, &balance);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    free(response.data);
    return balance;
}

std::string ConnectionManager::placeOrder(int action, int side, int price, int numContracts){
    std::string method = "POST";
    std::string path = "/trade-api/v2/portfolio/orders";
    CURL* curl = curl_easy_init();
    curl_slist *list = NULL;
    doAuth(method, path, list);
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, (baseUrl + path).c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    ResponseData response;
    response.data = (char *)malloc(1);
    response.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    char body[512];
    int order_id = 0;
    if(action == 1){ //buy
        if(side == 1){ //yes
            snprintf(body, 512, "{\"ticker\": \"%s\",\"side\": \"yes\",\"action\": \"buy\",\"count\": %d,\"yes_price\": %d,\"cancel_order_on_pause\": true}",ticker,numContracts,price);
        }
        if (side == 0){ // no
            snprintf(body, 512, "{\"ticker\": \"%s\",\"side\": \"no\",\"action\": \"buy\",\"count\": %d,\"yes_price\": %d,\"cancel_order_on_pause\": true}",ticker,numContracts,price);
        }
    }
    else if(action == 0){ //sell
        if (side == 1) {
             snprintf(body, 512, "{\"ticker\": \"%s\",\"side\": \"yes\",\"action\": \"sell\",\"count\": %d,\"yes_price\": %d,\"cancel_order_on_pause\": true}",ticker,numContracts,price);
        }
        if (side == 0){
             snprintf(body, 512, "{\"ticker\": \"%s\",\"side\": \"no\",\"action\": \"sell\",\"count\": %d,\"yes_price\": %d,\"cancel_order_on_pause\": true}",ticker,numContracts,price);
        }
    }

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    CURLcode mresult = curl_easy_perform(curl);
    if(mresult){
        std::cout << "Error creating in http request to place an order";
    }
    int httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode != 201){
        std::cout << "Order did not go through:" << std::endl;
        std::cout << response.data << std::endl;
        return "";
    }
    const char *key = "\"order_id\"";
    char delim = ',';
    char out[1024];
    parseJSON(response.data, key, delim, out, nullptr);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    free(response.data);
    return std::string(out);
}

int ConnectionManager::cancelOrder(std::string &orderId){
    std::string path = std::string("/trade-api/v2/portfolio/orders/") + orderId;
    std::string method = "DELETE";
    CURL* curl = curl_easy_init();
    curl_slist *list = NULL;
    doAuth(method, path, list);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, (baseUrl + path).c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    ResponseData response;
    response.data = (char *)malloc(1);
    response.size = 0;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    CURLcode mresult = curl_easy_perform(curl);
    if(mresult){
        std::cout << "Error creating hhtp request";
    }
    int httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode != 200){
        std::cout << "Error canceling the order: " << std::endl;
        std::cout <<response.data << std::endl;
    }
    free(response.data);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    return httpCode;
}

int ConnectionManager::createWebsocket(CURL *curl){
    std::string method = "GET";
    std::string path = "/trade-api/ws/v2";
    curl_slist *list = NULL;
    doAuth(method, path, list);

    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, "wss://api.elections.kalshi.com/trade-api/ws/v2");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 2L);
    CURLcode result = curl_easy_perform(curl);
    if (result) {
        std::cout << "Error creating the websocket connection: " << curl_easy_strerror(result) << std::endl;
        return 0;
    }
    return 1;
}

//Calculate the market timestamp and set it to ticker
void ConnectionManager::updateMarketTicker(char *ticker, std::string& market){
    const auto now = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(now);
    const auto local_time = std::localtime(&t);

    int year = local_time->tm_year - 100;
    int month = local_time->tm_mon;
    int day = local_time->tm_mday;
    int hour = local_time->tm_hour;
    int min = local_time->tm_min;
    
    std::vector<std::string> months = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    
    int min_f = 0;
    if (min >= 45) {
        min_f = 0;
    }
    else if (min >= 30){
        min_f = 45;
    }
    else if (min >= 15){
        min_f = 30;
    }
    else if (min >= 00){
        min_f = 15;
    }

    if (min_f == 0){
        hour++;
    }

    if (hour == 0){
        day++;
    }

    if (day == 0){
        month++;
    }

    if (month == 12){
        month = 0;
        year++;
    }
    std::string s_month = months[month];
    char s_day[3];
    if (day <= 9){
        snprintf(s_day, 3, "0%d", day);
    } 
    else {
        snprintf(s_day, 3, "%d", day);
    }
    char s_hour[3];
    if (hour <= 9){
        snprintf(s_hour, 3, "0%d", hour);
    }
    else{
        snprintf(s_hour, 3, "%d", hour);
    }
    char s_min[3];
    if (min_f == 0){
        snprintf(s_min, 3, "0%d", min_f);
    }
    else {
        snprintf(s_min, 3, "%d", min_f);
    }
    
    snprintf(ticker, 24, "%s-%d%s%s%s%s-%s", market.c_str(), year, s_month.c_str(), s_day, s_hour, s_min, s_min);
}

// Unsubscribes from a channel: returns 0 on success
int ConnectionManager::unsubscribeChannel(CURL *curl, int channel_id){
    this->ws_sub_id++;
    CURLcode send_msg = CURLE_OK;

    size_t s_offset = 0;
    char s_buffer[128];
    snprintf(s_buffer, 128, "{\"id\": %d, \"cmd\": \"unsubscribe\", \"params\": {\"sids\": [\"%d\"]}}", this->ws_sub_id, channel_id);
    while (!send_msg){
        size_t sent;
        send_msg = curl_ws_send(curl, s_buffer + s_offset, strlen(s_buffer) - s_offset, &sent, 0, CURLWS_TEXT);
        s_offset += sent;
        if(send_msg == CURLE_OK){
            return 0;
        }
        send_msg = CURLE_OK;
    }
    return -1;
}

int ConnectionManager::subscribeOrderbookUpdates(CURL *curl, char *data, size_t data_size){
    this->ws_sub_id++;
    CURLcode send_msg = CURLE_OK;
    size_t s_offset = 0;
    char s_buffer[256];
    snprintf(s_buffer, 256, "{\"id\": %d, \"cmd\": \"subscribe\", \"params\": {\"channels\": [\"orderbook_delta\"], \"market_ticker\": \"%s\"}}", this->ws_sub_id, ticker);

    while (!send_msg){
        size_t sent;
        send_msg = curl_ws_send(curl, s_buffer + s_offset, strlen(s_buffer) - s_offset, &sent, 0, CURLWS_TEXT);
        s_offset += sent;
        if(send_msg == CURLE_OK){
            break;
        }
        send_msg = CURLE_OK;
    }

    CURLcode receive_msg = CURLE_OK;
    size_t r_offset = 0;

    while (!receive_msg){
        size_t recv;
        const struct curl_ws_frame *meta;
        receive_msg = curl_ws_recv(curl, data + r_offset, data_size - r_offset, &recv, &meta);
        r_offset += recv;
        if (receive_msg == CURLE_AGAIN){
            receive_msg = CURLE_OK;
            continue;
        }
        if (receive_msg == CURLE_OK){
            if (meta->bytesleft == 0){
                return this->ws_sub_id;
            }
            if (meta->bytesleft > data_size - r_offset){
                receive_msg = CURLE_TOO_LARGE;
            }
            
        }
    }
    return -1;
}

int ConnectionManager::receiveWebsocketData(CURL* curl, pollfd *socket){
    CURLcode receive_msg = CURLE_OK;
    size_t r_offset = 0;
    char *data = (char*)malloc(4096);
    size_t data_size = 4096;
    memset((void*)data, 0, data_size);

    while(1){
        const struct curl_ws_frame *meta;
        size_t recv = 0;
        receive_msg = curl_ws_recv(curl, r_offset + data, data_size - r_offset, &recv, &meta);
       
        if (receive_msg == CURLE_AGAIN){
            int wait = poll(socket, 1, 10000);
            if (wait == 0){ //no data for more 10s
                std::cout << "Dead socket!" << std::endl;
                return -2;
            }
            continue;
        }
        r_offset += recv;
        if (receive_msg != 0){
            std::cout << "Error msg: " << receive_msg << std::endl;
            return -1;
        }
        if (meta->bytesleft > data_size - r_offset){
            std::cout << "The buffer was too small to intake the incoming frame" << std::endl;
            return -1; 
        }
        if (meta->bytesleft == 0){
            data[r_offset] = '\0';
            r_offset = 0;
            printf("Current frame: %s\n", data);
            memset((void*)data, 0, data_size);
        }
    }
    free(data);
}

int main(){
    ConnectionManager manager;

    CURL* curl = curl_easy_init();
    manager.createWebsocket(curl);
    char data[512];
    int result = manager.subscribeOrderbookUpdates(curl, data, 512);
   
    curl_socket_t socketfd;
    CURLcode get_fd = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &socketfd);
    struct pollfd socket;
    socket.fd = socketfd;
    socket.events = POLLIN;
    socket.revents = 0;
    printf("%s\n", data);
    manager.receiveWebsocketData(curl, &socket);

    curl_easy_cleanup(curl);
    return 0;
}

