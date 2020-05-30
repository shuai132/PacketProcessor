#include <random>
#include <cassert>
#include <ctime>

#include "PacketProcessor.h"
#include "log.h"

#define CHECK(b)    if (!(b)) FATAL("test failed"); else LOG("###PASSED###")

static void simpleUsage()  {
    LOG("Sample Usage:");
    PacketProcessor processor([&](uint8_t* data, size_t size) {
        LOG("Got packet: %zu, %s", size, std::string((char*)data, size).c_str());
    });
    auto payload = processor.pack("hello world");
    processor.feed(payload.data(), payload.size());
}

static void testCommon() {
    LOG("generate big data...");
    bool pass = false;
    std::string TEST_PAYLOAD;
    for (int i = 0; i < 1000; i++) {
        TEST_PAYLOAD += "helloworld";   // 10bytes
    }
    LOG("data generated, size:%zu", TEST_PAYLOAD.size());

    PacketProcessor processor([&](uint8_t* data, size_t size) {
        LOG("get payload size:%zu", size);
        if (std::string((char*)data, size) == TEST_PAYLOAD) {
            pass = true;
        }
    });

    LOG("packing...");
    auto payload = processor.pack(TEST_PAYLOAD);
    const uint32_t payloadSize = payload.size();
    LOG("payloadSize:%u", payloadSize);

    LOG("******test normal******");
    processor.feed(payload.data(), payloadSize);

    LOG("******test random******");
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
    CHECK(pass);
}

static void testSerious() {
    LOG("******test overflow******");
    bool pass = false;
    uint8_t testData = 0xAA;
    PacketProcessor processor([&](const uint8_t* data, size_t size) {
        LOG("get payload size:%zu", size);
        if (data[0] == testData) {
            pass = true;
        }
    });
    processor.setMaxBufferSize(1);
    const size_t fakeDataLen = 8;
    std::vector<uint8_t> payload = {
            // 错误数据(只有前部分)
            0x5A,
            0xA5,

            0x00,
            0x00,
            0x00,
            0x02,

            0xC0,
            0xC1,

            // 正式数据
            0x5A,
            0xA5,

            0x00,
            0x00,
            0x00,
            0x01,

            0xC0,
            0xC1,

            testData,

            0x3F,
            0x3E,
    };
    for (size_t i = 0; i < payload.size(); i++) {
        processor.feed(payload.data() + i, 1);
    }
    CHECK(pass);

    LOG("retest...");
    pass = false;
    processor.feed(payload.data(), payload.size());
    processor.feed(payload.data() + fakeDataLen, payload.size() - fakeDataLen);
    CHECK(pass);
}

int main() {
    simpleUsage();
    testCommon();
    testSerious();
    return 0;
}
