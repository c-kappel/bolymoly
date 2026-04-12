CPPC = clang++
CFLAGS = -std=c++20 -Wall -fsanitize=address
OPENSSL_CF = -I/opt/homebrew/opt/openssl@3.6/include/
OPENSSL_LF = -L/opt/homebrew/opt/openssl@3.6/lib/ -lcrypto
CURL_CF = -I/usr/local/include/
CURL_LF = -L/usr/local/lib/ -lcurl
.PHONY: clean

all: bot

clean:
	rm -rf src/kalshlib/*.o src/logic/*.o bot 

bot: src/kalshlib/kalshlib.o src/logic/orderbook.o src/kalshlib/kalshlib.h
	$(CPPC) $(CFLAGS) $< $(OPENSSL_LF) $(CURL_LF) -o $@

src/kalshlib/kalshlib.o: src/kalshlib/kalshlib.cpp src/kalshlib/kalshlib.h
	$(CPPC) $(CFLAGS) $(OPENSSL_CF) $(CURL_CF) -c $< -o $@

src/logic/orderbook.o: src/logic/orderbook.cpp src/kalshlib/kalshlib.h src/logic/orderbook.h
	$(CPPC) $(CFLAGS) $(OPENSSL_CF) -c $< -o $@