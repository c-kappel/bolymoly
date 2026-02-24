#include <iostream>
#include <cstdio>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <chrono>
#include "kalshlib.h"

ConnectionManager::ConnectionManager(){
    //Set public key and stage the private key
    publicKey = "2ec211ad-cc3c-4a92-b160-b9c0f8184ff8";
    FILE* fp = fopen("src/kalshlib/config.txt", "r");
    pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    ctx =  EVP_PKEY_CTX_new(pkey, NULL);
}

long long ConnectionManager::currentTimeMs(){
    const auto now = std::chrono::system_clock::now();
    //std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());
    std::time_t nowT = std::chrono::system_clock::to_time_t(now);
    long long nowMs = long(nowT * 1000);
    return nowMs;
}

double ConnectionManager::getBalance(std::vector<std::string> auth){

}

int main(){
    ConnectionManager manager;
    manager.currentTimeMs();
    return 0;
}

