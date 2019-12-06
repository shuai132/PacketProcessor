#include <random>
#include <cassert>
#include <ctime>

#include "PacketProcessor.h"
#include "log.h"

using std::placeholders::_1;
using std::placeholders::_2;

int main() {
    {
        LOG("Sample Usage:");
        PacketProcessor processor([&](uint8_t* data, size_t size) {
            LOG("Got packet: %zu, %s", size, std::string((char*)data, size).c_str());
        });
        auto payload = processor.pack("hello world");
        processor.feed(payload.data(), payload.size());
    }

    {
        LOG("generate big data...");
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
    }

    {
        LOG("******test header******");
        const std::string payload = "hello";
        PacketProcessor processor([&](uint8_t* data, size_t size) {
            LOG("get payload size:%zu", size);
            assert(std::string((char*)data, size) == payload);
        });

        {   // 试错
            uint8_t h1 = 0x5A;
            uint8_t h2 = 0xA5;
            processor.feed(&h1, 1);
            processor.feed(&h2, 1);
        }

        processor.packForeach(payload.data(), payload.size(),
                              std::bind(&PacketProcessor::feed, &processor, _1, _2));
    }

    {
        LOG("******test overflow******");
        const std::string payload = "123456";
        PacketProcessor processor([&](uint8_t* data, size_t size) {
            LOG("get payload size:%zu", size);
            assert(std::string((char*)data, size) == payload);
        });

        processor.setMaxBufferSize(payload.size());

        // 试错数据
        processor.packForeach(payload.data(), payload.size() + 1,
                              std::bind(&PacketProcessor::feed, processor, _1, _2));
        // 正确数据
        processor.packForeach(payload.data(), payload.size(),
                              std::bind(&PacketProcessor::feed, processor, _1, _2));
    }

    return 0;
}
