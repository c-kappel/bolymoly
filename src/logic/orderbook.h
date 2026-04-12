
struct PriceLevel {
    double price;
    int numContracts;
};

class Orderbook {
    private:
        PriceLevel levels[1000];
    public: 
        Orderbook();

        void parseSnapshot(char *orderbookBuffer, int position);

        void updateOrderbook(PriceLevel level);

        PriceLevel readCurrentPrice();

};