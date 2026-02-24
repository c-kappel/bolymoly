#include <string>
#include <vector>

class ConnectionManager{
    public:
    std::string publicKey;
    EVP_PKEY* pkey;
    EVP_PKEY_CTX* ctx;

    ConnectionManager();

    std::string getPubkey();

    std::string getPrivatekey();
    
    long long currentTimeMs();

    double getBalance(std::vector<std::string> auth);

    int placeOrder(std::vector<std::string> auth, std::string ticker, std::string action, std::string side, int maxCost, int numContracts); //cancel_order_on_pause = true

    int cancelOrder(std::vector<std::string> auth, int orderId, int subAccount);

    int createWebsocket(std::vector<std::string> auth, std::string marketTicker);

    int keepWebsocketAlive(std::vector<std::string> auth);

    int getOrderbookData(std::vector<std::string> auth, std::string marketTicker);
};
