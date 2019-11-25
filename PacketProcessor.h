#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <functional>

class PacketProcessor {
    using OnPacketHandle = std::function<void(std::string payload)>;

public:
    explicit PacketProcessor(OnPacketHandle handle = nullptr, bool useCrc = false);

    explicit PacketProcessor(bool useCrc = false);

public:
    void setOnPacketHandle(const OnPacketHandle& handle);

    void setUseCrc(bool useCrc);

    void setBufferSize(uint32_t maxBufSize);

    /**
     * 打包
     * @param data 视为uint8_t*
     * @param size
     * @return
     */
    std::vector<uint8_t> pack(const void* data, uint32_t size);

    std::vector<uint8_t> pack(const std::string& data);

    /**
     * 给定数据打包 并把包字节遍历
     * @param data 视为uint8_t*
     * @param size
     * @param handle
     */
    void packForeach(const void* data, uint32_t size, const std::function<void(uint8_t* data, size_t size)>& handle);

    /**
     * 只管送数据 会自动解析当有合适的包时会回调onPacketHandle_
     * @param data
     * @param size
     */
    void feed(const uint8_t* data, size_t size);

private:
    size_t getDataPos();

    size_t getDataLength();

    std::string getPayload();

    size_t getLenPos();

    void tryUnPacket();

    bool checkCrc();

    size_t getNextPacketPos();

    void restart();

private:
    OnPacketHandle onPacketHandle_;
    bool useCrc_;
    static const int HEADER_LEN = 2;
    static const int LEN_BYTES = 4;
    static const int CHECK_LEN = 2;
    static const uint32_t MAX_BUFFER_SIZE_DEFAULT = 1024 * 1024 * 1;    // 1MBytes

    std::vector<uint8_t> buffer_;   // 数据缓存
    uint32_t maxBufferSize = MAX_BUFFER_SIZE_DEFAULT; // 最大缓存字节数
    size_t lenPos_ = 0;             // 长度位置
    size_t headerLen_ = 0;          // 包头中的长度
};
