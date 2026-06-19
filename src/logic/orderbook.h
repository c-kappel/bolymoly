#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <iostream>

class Orderbook {
    public:
        float up[10000];
        float down[10000];
 
        Orderbook();

        void parseSnapshot(char *orderbookBuffer, size_t position, std::atomic<unsigned> count);

        void updateOrderbook(double level);

        void readCurrentPrice();

};
#endif