#include <string>
#include <vector>

class ConnectionManager{
    public:
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

    int createWebsocket(CURL &curl);

    int getOrderbookData(CURL &curl);

    int keepWebsocketAlive();

};
