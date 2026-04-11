
struct PriceLevel {
    double price;
    int numContracts;
}

class Orderbook {
    private:
        PriceLevel levels[1000];
    public: 
        Orderbook();

        void updateOrderbook(PriceLevel level);

        PriceLevel readCurrentPrice();

}