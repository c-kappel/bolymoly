#ifndef KALSHLIB_H
#define KALSHLIB_H

#include <string>
#include <vector>
#include <curl/curl.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <poll.h>

class ConnectionManager{
    private:
        size_t r_offset;
        int ws_sub_id;
        char ticker[24];
        std::string publicKey;
        EVP_PKEY* pkey;
        EVP_PKEY_CTX* ctx;
        std::string baseUrl;
        char *orderbookMsg;
    public:
        ConnectionManager();
        
        std::string currentTimeMs();
        
        std::string generateMsg(std::string &time, std::string &method, std::string &path);

        std::string signMsg(std::string &msg, EVP_PKEY* pkey);

        void doAuth(std::string method, std::string path, curl_slist *&list);

        int getBalance();

        std::string placeOrder(int action, int side, int maxCost, int numContracts);

        int cancelOrder(std::string &orderId);

        void updateMarketTicker(char *ticker, std::string& market);

        int createWebsocket(CURL *curl);

        int subscribeOrderbookUpdates(CURL *curl, char *data, size_t size_data);

        int unsubscribeChannel(CURL *curl, int channel_id);

        int receiveWebsocketData(CURL *curl, pollfd *socket);

};

#endif
