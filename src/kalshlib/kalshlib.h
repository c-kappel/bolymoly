#include <string>
#include <vector>
#include <curl/curl.h>

class ConnectionManager{
    public:
    int ws_sub_id;
    char ticker[24];
    std::string publicKey;
    EVP_PKEY* pkey;
    EVP_PKEY_CTX* ctx;
    std::string baseUrl;

    ConnectionManager();
    
    std::string currentTimeMs();
    
    std::string generateMsg(std::string &time, std::string &method, std::string &path);

    std::string signMsg(std::string &msg, EVP_PKEY* pkey);

    void doAuth(std::string method, std::string path, curl_slist *&list);

    std::string getBalance();

    std::string placeOrder(const char *ticker, int action, int side, int maxCost, int numContracts);

    int cancelOrder(std::string &orderId);

    int updateMarketTicker(char *ticker);

    int createWebsocket(CURL *curl);

    int subscribeOrderbookUpdates(CURL *curl, const char *buffer, char *data, size_t size_data);

    int unsubscribeChannel(CURL *curl, int channel_id);

    int receiveWebsocketData(CURL *curl, pollfd *socket);

};
