#include <iostream>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include "kalshlib.h"

ConnectionManager::ConnectionManager(){
    //Set public key and stage the private key
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
std::vector<char*> parseJSON(char *input, const char* keys[], size_t keyLengths[], size_t numKeys){
    std::vector<char*> parsed;
    for(size_t i = 0; i < numKeys; i++){
        char* value = strstr(input, keys[i]);
        if(!value){
            std::cout << "Errr finding the key: " << keys[i];
            return parsed;
        }
        value += keyLengths[i] + 2;
        char* cutOff = strchr(value, ',');
        if(!cutOff){
            std::cout << "Errr finding the key cutoff for key: " << keys[i];
            return parsed;
        }
        char* buff = (char *)malloc(64);
        int pos = 0;
        while(value != cutOff){
            buff[pos] = *value;
            value++;
            pos++;
        }
        if(buff[0] == '\"'){
            buff++;
            buff[pos-1] = '\0';

        }
        else{
            buff[pos] = '\0';
        } 
        parsed.push_back(buff);
    }

    return parsed;
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

std::string ConnectionManager::getBalance(){
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
    const char *keys[] = {"balance"};
    size_t keySizes[] = {7};
    std::vector<char *> parsed = parseJSON(response.data, keys, keySizes, (size_t)1);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    free(response.data);
      if(parsed.size() > 0){
        std::string balance = parsed[0];
        free(parsed[0]);
        return balance;
    }
    std:: cout<<"error parsing the balance.";
    return "";
}

std::string ConnectionManager::placeOrder(const char *ticker, int action, int side, int price, int numContracts){
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
        std::cout << "Error creating hhtp request";
    }
    const char *keys[] = {"order_id"};
    size_t sizes[] = {8};
    std::vector<char *> results = parseJSON(response.data, keys, sizes, (size_t)1);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    free(response.data);
    std::string orderId = results[0];
    free(results[0]);
    return orderId;
    return "";
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

int ConnectionManager::subscribeTickerUpdates(CURL *curl, const char *ticker, char *data, size_t data_size){
    CURLcode send_msg = CURLE_OK;
    size_t s_offset = 0;
    char s_buffer[256];
    snprintf(s_buffer, 256, "{\"id\": 1, \"cmd\": \"subscribe\", \"params\": {\"channels\": [\"ticker\"], \"market_ticker\": \"%s\"}}", ticker);

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
                return 1;
            }
            if (meta->bytesleft > data_size - r_offset){
                receive_msg = CURLE_TOO_LARGE;
            }
            
        }
    }
    return 0;
}

int ConnectionManager::receiveWebsocketData(CURL* curl){
    CURLcode receive_msg = CURLE_OK;
    char data[512];
    size_t data_size = 512;
    while(1){
        const struct curl_ws_frame *meta;
        size_t r_offset = 0;
        size_t recv;
        receive_msg = curl_ws_recv(curl, r_offset + data, data_size - r_offset, &recv, &meta);
        r_offset += recv;
        if (receive_msg == CURLE_AGAIN){
            std::cout << "No data yet :()" << std::endl;
            continue;
        }
        if (meta->bytesleft > data_size - r_offset){
            std::cout << "The buffer was too small to intake the incoming frame" << std::endl;
            return -1; 
        }
        if (meta->bytesleft == 0){
            printf("Current frame: %s\n", data);
        }
    }
}

int main(){
    //MARKET FORMAT: "KXSOL15M-26FEB251815-15"
    ConnectionManager manager;
    CURL* curl = curl_easy_init();
    manager.createWebsocket(curl);
    char data[512];
    int result = manager.subscribeTickerUpdates(curl, "KXSOL15M-26MAR120145-15", data, 512);
    printf("%s\n", data);
    manager.receiveWebsocketData(curl);
    return 0;
}

