#include <iostream>
#include "../kalshlib/kalshlib.h"
#include "orderbook.h"

Orderbook::Orderbook(){
    for (int i = 0; i < 10000; ++i){
        PriceLevel p = {i/10, 0};
        levels[i] = p;
    }
}

/*
Parses the buffer from 
*/
Orderbook::parseSnapshot(char *orderbookBuffer, int bufferPosition){

}