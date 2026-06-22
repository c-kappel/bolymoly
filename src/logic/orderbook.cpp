#include "../kalshlib/kalshlib.h"
#include "orderbook.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>

namespace {
constexpr size_t kBookLevels = 10000;
constexpr size_t kSlotsPerBuffer = BUFFER_SIZE / INDEX_SIZE;

char* skipWhitespace(char* ptr, char* end) {
    while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
        ++ptr;
    }
    return ptr;
}
// finds the given key
char* findToken(char* begin, char* end, const char* token) {
    const size_t tokenLen = std::strlen(token);
    if (tokenLen == 0 || begin >= end) {
        return nullptr;
    }
    for (char* ptr = begin; ptr + tokenLen <= end; ++ptr) {
        if (std::memcmp(ptr, token, tokenLen) == 0) {
            return ptr;
        }
    }
    return nullptr;
}

bool readFloatToken(char*& ptr, char* end, float& value) {
    ptr = skipWhitespace(ptr, end);
    if (ptr >= end) {
        return false;
    }

    if (*ptr == '"') {
        ++ptr;
    }

    char token[64];
    size_t len = 0;
    char* start = ptr;

    while (ptr < end) {
        const char c = *ptr;
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E') {
            if (len + 1 < sizeof(token)) {
                token[len++] = c;
            }
            ++ptr;
            continue;
        }
        break;
    }

    if (ptr < end && *ptr == '"') {
        ++ptr;
    }

    if (ptr == start || len == 0) {
        return false;
    }

    token[len] = '\0';
    value = std::strtof(token, nullptr);
    return true;
}

bool readStringToken(char*& ptr, char* end, char* out, size_t outSize) {
    ptr = skipWhitespace(ptr, end);
    if (ptr >= end || *ptr != '"') {
        return false;
    }

    ++ptr;
    size_t len = 0;
    while (ptr < end && *ptr != '"') {
        if (len + 1 < outSize) {
            out[len++] = *ptr;
        }
        ++ptr;
    }

    if (ptr >= end || *ptr != '"') {
        return false;
    }

    ++ptr;
    out[len] = '\0';
    return true;
}

char* findFieldValue(char* begin, char* end, const char* key) {
    char* keyPos = findToken(begin, end, key);
    if (keyPos == nullptr) {
        return nullptr;
    }

    char* ptr = keyPos + std::strlen(key);
    ptr = skipWhitespace(ptr, end);
    if (ptr >= end || *ptr != ':') {
        return nullptr;
    }

    ++ptr;
    return skipWhitespace(ptr, end);
}

void parseSide(char* begin, char* end, const char* key, float* book_side) {
    char* ptr = findFieldValue(begin, end, key);
    if (ptr == nullptr) {
        return;
    }

    if (ptr >= end || *ptr != '[') {
        return;
    }

    ++ptr;
    while (ptr < end) {
        ptr = skipWhitespace(ptr, end);
        if (ptr >= end) {
            return;
        }
        if (*ptr == ']') {
            return;
        }
        if (*ptr != '[') {
            ++ptr;
            continue;
        }

        ++ptr;
        float price = 0.0f;
        float size = 0.0f;

        if (!readFloatToken(ptr, end, price)) {
            return;
        }

        ptr = skipWhitespace(ptr, end);
        if (ptr >= end || *ptr != ',') {
            return;
        }

        ++ptr;
        if (!readFloatToken(ptr, end, size)) {
            return;
        }

        ptr = skipWhitespace(ptr, end);
        if (ptr >= end || *ptr != ']') {
            return;
        }

        const int level = static_cast<int>(std::lround(price * 10000.0f));
        if (level >= 0 && level < static_cast<int>(kBookLevels)) {
            book_side[level] = size;
        }

        ++ptr;
        ptr = skipWhitespace(ptr, end);
        if (ptr < end && *ptr == ',') {
            ++ptr;
        }
    }
}

void applyDelta(char* begin, char* end, float* book_side) {
    char* pricePtr = findFieldValue(begin, end, "\"price_dollars\"");
    char* deltaPtr = findFieldValue(begin, end, "\"delta_fp\"");
    if (pricePtr == nullptr || deltaPtr == nullptr) {
        return;
    }

    float price = 0.0f;
    float delta = 0.0f;
    if (!readFloatToken(pricePtr, end, price)) {
        return;
    }
    if (!readFloatToken(deltaPtr, end, delta)) {
        return;
    }

    const int level = static_cast<int>(std::lround(price * 10000.0f));
    if (level < 0 || level >= static_cast<int>(kBookLevels)) {
        return;
    }

    book_side[level] += delta;
    if (book_side[level] < 0.0f) {
        book_side[level] = 0.0f;
    }
}
} // namespace

Orderbook::Orderbook() {
    std::fill_n(yes_fp, kBookLevels, 0.0f);
    std::fill_n(no_fp, kBookLevels, 0.0f);
}

// Parses the buffer from the websocket, translates into
void Orderbook::translateWebsocketData(char *orderbookBuffer, std::atomic<unsigned> &count, bool &snapshot_f) {
    unsigned consumer = 0;
    while (1) {
        while (count.load() == 0) {
            // busy waiting when there are no messages
        }

        if (snapshot_f) {
            parseSnapshot(orderbookBuffer, consumer + 4);
            count.fetch_sub(4, std::memory_order_relaxed);
            consumer = (consumer + 4) % kSlotsPerBuffer;
            snapshot_f = false;
        }
        else {
            parseUpdate(orderbookBuffer + (consumer * INDEX_SIZE), consumer);
            count.fetch_sub(1, std::memory_order_relaxed);
            consumer = (consumer + 1) % kSlotsPerBuffer;
        }
        printBook();
    }
}

void Orderbook::parseSnapshot(char *orderbookBuffer, size_t position) {
    if (orderbookBuffer == nullptr || position == 0) {
        return;
    }

    const size_t snapshotBytes = std::min(position * INDEX_SIZE, static_cast<size_t>(BUFFER_SIZE));
    char* begin = orderbookBuffer;
    char* end = orderbookBuffer + snapshotBytes;
    parseSide(begin, end, "\"yes_dollars_fp\"", yes_fp);
    parseSide(begin, end, "\"no_dollars_fp\"", no_fp);
}

void Orderbook::parseUpdate(char *orderbookBuffer, size_t position) {
    if (orderbookBuffer == nullptr) {
        return;
    }

    (void)position;
    const size_t updateBytes = INDEX_SIZE;
    char* begin = orderbookBuffer;
    char* end = orderbookBuffer + updateBytes;

    char side[8];
    char* sidePtr = findFieldValue(begin, end, "\"side\"");
    if (sidePtr == nullptr || !readStringToken(sidePtr, end, side, sizeof(side))) {
        return;
    }

    if (std::strcmp(side, "yes") == 0) {
        applyDelta(begin, end, yes_fp);
    }
    else if (std::strcmp(side, "no") == 0) {
        applyDelta(begin, end, no_fp);
    }
}

void Orderbook::updateOrderbook(double level) {
    (void)level;
}

void Orderbook::readCurrentPrice() {
}

void Orderbook::printBook() {
    for (int i = 0; i < std::size(this->yes_fp) / 2; i+=100) {
        std::cout << "Level: " << i / 10000.0f << " " << "Amount: " << yes_fp[i] << std::endl;
    }
}
