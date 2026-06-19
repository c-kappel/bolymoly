#include "orderbook.h"
#include "../kalshlib/kalshlib.h"
#include <iostream>

int main(){
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
    manager.receiveWebsocketData(curl, &socket, count);
    
    curl_easy_cleanup(curl);
}