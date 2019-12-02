#include <random>
#include <cassert>
#include <ctime>

#include "log.h"
#include "PacketProcessor.h"

int main() {
    LOG("generate data...");
    std::string TEST_PAYLOAD;
    for (int i = 0; i < 1000; i++) {
        TEST_PAYLOAD += "helloworld";   // 10bytes
    }
    LOG("data generated, size:%zu", TEST_PAYLOAD.size());

    PacketProcessor processor([&](uint8_t* data, size_t size) {
        LOG("get payload size:%zu", size);
        assert(std::string((char*)data, size) == TEST_PAYLOAD);
    });

    LOG("packing...");
    auto payload = processor.pack(TEST_PAYLOAD);
    const uint32_t payloadSize = payload.size();
    LOG("payloadSize:%u", payloadSize);

    LOG("******test1******");
    processor.feed(payload.data(), payloadSize);

    LOG("******test2******");
    uint32_t sendSize = 0;
    std::default_random_engine generator(time(nullptr));
    std::uniform_int_distribution<int> dis(1, 10);
    auto random = std::bind(dis, generator);
    while (sendSize < payloadSize) {
        uint32_t randomSize = random();
        if (payloadSize - sendSize < randomSize) {
            randomSize = payloadSize - sendSize;
        }
        processor.feed(payload.data() + sendSize, randomSize);
        sendSize += randomSize;
    }
    LOG("allSize:%u, sendSize:%u", payloadSize, sendSize);
    return 0;
}
