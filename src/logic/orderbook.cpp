#include "../kalshlib/kalshlib.h"
#include "orderbook.h"


// Parses the buffer from the websocket, translates into 
void Orderbook::parseSnapshot(char *orderbookBuffer, size_t bufferPosition, std::atomic<unsigned> count){
    while (1) {
        while(count.load() == 0); // busy waiting when there are no messages
        
    }
}

