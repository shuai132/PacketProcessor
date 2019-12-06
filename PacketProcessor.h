#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <functional>

class PacketProcessor {
    using OnPacketHandle = std::function<void(uint8_t* data, size_t size)>;

public:
    explicit PacketProcessor(OnPacketHandle handle = nullptr, bool useCrc = false);

    explicit PacketProcessor(bool useCrc = false);

public:
    void setOnPacketHandle(const OnPacketHandle& handle);

    /**
     * 设置对数据是否启用CRC 否则对数据长度CRC
     * @param useCrc
     */
    void setUseCrc(bool useCrc);

    void setMaxBufferSize(uint32_t size);

    void clearBuffer();

    /**
     * 打包数据
     * @param data 视为uint8_t*
     * @param size
     * @return
     */
    std::vector<uint8_t> pack(const void* data, uint32_t size);

    std::vector<uint8_t> pack(const std::string& data);

    /**
     * 遍历打包数据
     * @param data 视为uint8_t*
     * @param size
     * @param handle
     */
    void packForeach(const void* data, uint32_t size, const std::function<void(uint8_t* data, size_t size)>& handle);

    /**
     * 送数据 自动解析出数据包时回调onPacketHandle_
     * @param data
     * @param size
     */
    void feed(const uint8_t* data, size_t size);

private:
    size_t getDataPos();

    size_t getDataSize();

    uint8_t* getPayloadPtr();

    bool findHeader();

    void tryUnpack();

    bool checkCrc();

    size_t getNextPacketPos();

    void restart(uint32_t pos);

private:
    OnPacketHandle onPacketHandle_;
    bool useCrc_;

    static const uint8_t H_1 = 0x5A;
    static const uint8_t H_2 = 0xA5;
    static const unsigned int HEADER_LEN = 2;
    static const unsigned int LEN_CRC_B = 2;
    static const unsigned int LEN_BYTES = 4 + LEN_CRC_B;
    static const unsigned int CHECK_LEN = 2;
    static const unsigned int ALL_HEADER_LEN = HEADER_LEN + LEN_BYTES + CHECK_LEN;

    std::vector<uint8_t> buffer_;   // 数据缓存
    uint32_t maxBufferSize_ = 1024 * 1024 * 1;  // 最大缓存字节数 默认1MBytes
    bool findHeader_ = false;       // 找到包头
    size_t dataSize_ = 0;           // 解析出的数据净长度
};
