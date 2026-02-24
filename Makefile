CPPC = clang++
CFLAGS = -std=c++17 
OPENSSL_CF = -I/opt/homebrew/opt/openssl@3.6/include/
OPENSSL_LF = -L/opt/homebrew/opt/openssl@3.6/lib/ -lcrypto
.PHONY: clean

all: kalshlib.out

clean:

kalshlib.out: src/kalshlib/kalshlib.o src/kalshlib/kalshlib.h
	$(CPPC) $(CFLAGS) $< $(OPENSSL_LF) -o $@


src/kalshlib/kalshlib.o: src/kalshlib/kalshlib.cpp src/kalshlib/kalshlib.h
	$(CPPC) $(CFLAGS) $(OPENSSL_CF) -c $< -o $@