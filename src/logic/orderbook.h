#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <atomic>
#include <cstddef>
#include <iostream>

class Orderbook {
    public:
        float yes_fp[10000];
        float no_fp[10000];
 
        Orderbook();

        void translateWebsocketData(char *orderbookBuffer, std::atomic<unsigned> &count, bool &snapshot_f);

    private:
        void parseSnapshot(char *orderbookBuffer, size_t position);
        void parseUpdate(char *orderbookBuffer, size_t position);
        void printBook();

    public:
        void updateOrderbook(double level);

        void readCurrentPrice();
};
#endif
