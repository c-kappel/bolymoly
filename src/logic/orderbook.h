#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <iostream>

struct PriceLevel {
    double price;
    int numContracts;
};

class Orderbook {
    public:
        PriceLevel levels[10000];
 
        Orderbook();

        void parseSnapshot(char *orderbookBuffer, size_t position);

        void updateOrderbook(PriceLevel level);

        PriceLevel readCurrentPrice();

};
#endif