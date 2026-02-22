#include <string>
#include <vector>

class ConnectionManager{
double getBalance(std::vector<std::string> auth);

int placeOrder(std::vector<std::string> auth, std::string ticker, std::string action, std::string side, int maxCost, int numContracts); //cancel_order_on_pause = true

int cancelOrder(std::vector<std::string> auth, int orderId, int subAccount);

int createWebsocket(std::vector<std::string> auth, std::string marketTicker);

int keepWebsocketAlive(std::vector<std::string> auth);

int getOrderbookData(std::vector<std::string> auth, std::string marketTicker);
};
