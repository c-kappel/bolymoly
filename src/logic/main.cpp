#include "orderbook.h"
#include "../kalshlib/kalshlib.h"
#include <iostream>
#include <thread>

int main(){
    // orderbook buffer 
    char orderbookMsg[BUFFER_SIZE] = {0};

    // manager code 
    // TODO: make ts way cleaner
    ConnectionManager manager{"KXSOL15M"};
    CURL* curl = curl_easy_init();
    manager.createWebsocket(curl);
    char data[512];
    int result = manager.subscribeOrderbookUpdates(curl, data, 512);
    curl_socket_t socketfd;
    curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &socketfd);
    struct pollfd socket;
    socket.fd = socketfd;
    socket.events = POLLIN;
    socket.revents = 0;
    std::atomic<unsigned> count{0};
    bool snapshot_f = false;

    std::thread producer(&ConnectionManager::receiveWebsocketData, &manager, orderbookMsg, curl, &socket, std::ref(count), std::ref(snapshot_f));

    // orderbook code 
    // todo: make way cleaner
    Orderbook orderbook{};
    std::thread consumer(&Orderbook::translateWebsocketData, &orderbook, orderbookMsg, std::ref(count), std::ref(snapshot_f));
    
    producer.join();
    consumer.join();
    curl_easy_cleanup(curl);
}