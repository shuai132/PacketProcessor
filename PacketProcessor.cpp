#include <utility>

#include "PacketProcessor.h"
#include "crc/checksum.h"
#include "log.h"

PacketProcessor::PacketProcessor(OnPacketHandle handle, bool useCrc)
        : onPacketHandle_(std::move(handle)), useCrc_(useCrc) {
}

PacketProcessor::PacketProcessor(bool useCrc)
        : onPacketHandle_(nullptr), useCrc_(useCrc) {
}

void PacketProcessor::setOnPacketHandle(const OnPacketHandle& handle) {
    onPacketHandle_ = handle;
}

std::vector<uint8_t> PacketProcessor::pack(const void* data, uint32_t size) {
    uint32_t payloadSize = HEADER_LEN + LEN_BYTES + size + CHECK_LEN;
    std::vector<uint8_t> payload;
    payload.reserve(payloadSize);
    packForeach(data, size, [&](uint8_t v) {
        payload.push_back(v);
    });
    return payload;
}

std::vector<uint8_t> PacketProcessor::pack(const std::string& data) {
    return pack(data.data(), data.length());
}

void PacketProcessor::packForeach(const void* data, uint32_t size, const std::function<void(uint8_t)>& handle) {
    uint32_t payloadSize = HEADER_LEN + LEN_BYTES + size + CHECK_LEN;
    std::vector<uint8_t> payload(payloadSize);
    handle(0x5A);
    handle(0xA5);
    uint32_t dataSize = size + CHECK_LEN;
    handle((dataSize & 0xff000000) >> 8 * 3);
    handle((dataSize & 0x00ff0000) >> 8 * 2);
    handle((dataSize & 0x0000ff00) >> 8 * 1);
    handle((dataSize & 0x000000ff) >> 8 * 0);
    for(size_t i = 0; i < size; i++) {
        handle(((uint8_t*)data)[i]);
    }
    uint16_t crcSum = useCrc_ ? crc_16((const unsigned char *)data, size) : 0;
    handle((crcSum & 0xff00) >> 8 * 1);
    handle((crcSum & 0x00ff) >> 8 * 0);
}

/**
 * 约定形式: 包头2字节(0x5AA5)+长度4字节(大端序)+数据+校验2字节(数据CRC16)
 * 具体算法: 使用buffer_缓存一包数据...
 * @param data
 * @param size
 */
void PacketProcessor::feed(const uint8_t* data, size_t size) {
    if (size == 0) return;

    // 缓存数据
    for(size_t i = 0; i < size; i++) {
        buffer_.push_back(data[i]);
    }

    // 尝试解包
    tryUnPacket();
}

size_t PacketProcessor::getDataPos() {
    assert(buffer_.size() != 0);
    assert(buffer_.size() >= headerLen_);
    return getLenPos() + LEN_BYTES;
}

/**
 * @return 数据长度 只包含数据 不包括头、长度、校验等
 */
size_t PacketProcessor::getDataLength() {
    assert(buffer_.size() != 0);
    assert(buffer_.size() >= headerLen_);
    return headerLen_ - CHECK_LEN;  // 减去校验
}

std::string PacketProcessor::getPayload() {
    assert(buffer_.size() != 0);
    assert(buffer_.size() >= headerLen_);
    return std::string((char*)buffer_.data() + getDataPos(), getDataLength());
}

/**
 * 找包头位置 buffer除去包头后的位置 也即长度的位置 为0说明暂未找到包头
 */
size_t PacketProcessor::getLenPos() {
    if (lenPos_ != 0) {
        return lenPos_;
    }
    for(size_t i = 0; i < buffer_.size(); i++) {
        uint8_t data = buffer_[i];
        if (data == 0x5A) {
            if (buffer_.size() > i + 1) {
                if (buffer_[i + 1] == 0xA5) {
                    lenPos_ = i + HEADER_LEN;
                    return lenPos_;
                }
            }
        }
    }
    return 0;
}

void PacketProcessor::tryUnPacket() {
    size_t size = 0;
    auto pos = getLenPos();
    if (pos == 0) return;

    // 等足够四字节时开始计算长度
    if (buffer_.size() - pos < LEN_BYTES) return;
    if (headerLen_ == 0) {
        for(size_t i = 0; i < LEN_BYTES; i++) {
            size += buffer_[lenPos_ + i] << (LEN_BYTES - i - 1) * 8;
        }
        headerLen_ = size;
        LOGD("headerLen_=%zu", headerLen_);
    }

    // 判断长度是否足够
    if (buffer_.size() >= HEADER_LEN + LEN_BYTES + headerLen_ && onPacketHandle_) {
        if (checkCrc()) {
            onPacketHandle_(getPayload());
        } else {
            LOGE("checkCrc false");
        }
        restart();
    }
}

bool PacketProcessor::checkCrc() {
    if (not useCrc_) return true;

    uint8_t* buffer = buffer_.data();

    uint8_t* dataPos = buffer + getDataPos();
    uint32_t dataSize = getDataLength();
    uint8_t* crcPos = dataPos + dataSize;

    uint16_t crcSum = crc_16(dataPos, dataSize);
    uint16_t crcData = 0;
    for (int i = 0; i < CHECK_LEN; i++) {
        crcData += crcPos[i] << 8 * (CHECK_LEN - 1 - i);
    }
    LOGD("checkCrc: 0x%02X, 0x%02X", crcData, crcSum);
    return crcData == crcSum;
}

size_t PacketProcessor::getNextPacketPos() {
    return getDataPos() + headerLen_;
}

void PacketProcessor::restart() {
    buffer_.assign(buffer_.cbegin() + getNextPacketPos(), buffer_.cend());
    lenPos_ = 0;
    headerLen_ = 0;

    // 每次解包成功后 要继续尝试解包 因为缓冲可能包含多个包
    tryUnPacket();
}
