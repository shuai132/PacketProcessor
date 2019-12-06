#include <utility>
#include <cassert>

#include "PacketProcessor.h"
#include "crc/checksum.h"
#include "log.h"

#if 0
#define LOGV            LOGI
#else
#define LOGV(fmt, ...)  ((void)0)
#endif

#define FORS(i, s, n)   for(std::remove_reference<std::remove_const<typeof(n)>::type>::type i = s; i < n; i++)
#define FOR(i, n)       FORS(i, 0, n)

template <typename T>
static uint16_t calCrc(T param) {
    constexpr int size = sizeof(param);
    uint8_t tmp[size];
    FOR (i, size) {
        tmp[i] = param >> 8u * (size - i - 1);
    }
    return crc_16(tmp, size);
}

PacketProcessor::PacketProcessor(OnPacketHandle handle, bool useCrc)
        : onPacketHandle_(std::move(handle)), useCrc_(useCrc) {
}

PacketProcessor::PacketProcessor(bool useCrc)
        : onPacketHandle_(nullptr), useCrc_(useCrc) {
}

void PacketProcessor::setOnPacketHandle(const OnPacketHandle& handle) {
    onPacketHandle_ = handle;
}

void PacketProcessor::setUseCrc(bool enable) {
    useCrc_ = enable;
}

void PacketProcessor::setMaxBufferSize(uint32_t size) {
    maxBufferSize_ = size + ALL_HEADER_LEN;
    clearBuffer();
}

void PacketProcessor::clearBuffer() {
    typeof(buffer_) tmp;
    tmp.swap(buffer_);
    findHeader_ = false;
    dataLen_ = 0;
}

std::vector<uint8_t> PacketProcessor::pack(const void* data, uint32_t size) {
    uint32_t payloadSize = HEADER_LEN + LEN_BYTES + size + CHECK_LEN;
    std::vector<uint8_t> payload;
    payload.reserve(payloadSize);
    packForeach(data, size, [&](uint8_t* data, size_t size) {
        while (size--) {
            payload.push_back(*data++);
        }
    });
    return payload;
}

std::vector<uint8_t> PacketProcessor::pack(const std::string& data) {
    return pack(data.data(), data.length());
}

void PacketProcessor::packForeach(const void* data, uint32_t size, const std::function<void(uint8_t* data, size_t size)>& handle) {
    uint8_t tmp[4];

    tmp[0] = H_1;
    tmp[1] = H_2;
    handle(tmp, 2);

    uint32_t dataSize = size + CHECK_LEN;
    tmp[0] = (dataSize & 0xff000000) >> 8 * 3;
    tmp[1] = (dataSize & 0x00ff0000) >> 8 * 2;
    tmp[2] = (dataSize & 0x0000ff00) >> 8 * 1;
    tmp[3] = (dataSize & 0x000000ff) >> 8 * 0;
    handle(tmp, 4);

    uint16_t sizeCrc = crc_16(tmp, 4);
    tmp[0] = (sizeCrc & 0xff00) >> 8 * 1;
    tmp[1] = (sizeCrc & 0x00ff) >> 8 * 0;
    handle(tmp, 2);

    handle((uint8_t*)data, size);

    uint16_t crcSum = useCrc_ ? crc_16((uint8_t*)data, size) : 0xAA55;
    tmp[0] = (crcSum & 0xff00) >> 8 * 1;
    tmp[1] = (crcSum & 0x00ff) >> 8 * 0;
    handle(tmp, 2);
}

/**
 * 约定形式: 包头2字节(0x5AA5)+长度4字节(大端序)+长度校验2字节(长度CRC16)+数据+校验2字节(数据CRC16)
 * @param data
 * @param size
 */
void PacketProcessor::feed(const uint8_t* data, size_t size) {
    if (size == 0) return;
//    LOGI("feed size: %u", size);
//    FOR(i, size) {
//        LOGI("feed: %02X", data[i]);
//    }

    // 缓存数据(当遇到包头后才开始缓存)
    size_t startPos = 0;
    if (buffer_.empty()) {
        START_HEADER:
        FOR (i, size) {
            if (data[i] == H_1) {
                if (i + 1 < size) {
                    if (data[i+1] == H_2) {
                        startPos = i;
                        goto START_BUFFER;
                    } else {
                        continue;
                    }
                } else {
                    buffer_.push_back(data[i]);
                    return;
                }
            }
        }
    } else if (buffer_.size() == 1) {
        assert(buffer_[0] == H_1);
        if (data[0] == H_2) goto START_BUFFER;
        else {
            buffer_.clear();
            goto START_HEADER;
        }
    } else {
        START_BUFFER:
        if (buffer_.size() + size - startPos > maxBufferSize_) {
            LOGE("buffer size overflow, now: %zu, max: %u", buffer_.size() + size, maxBufferSize_);
            clearBuffer();
            return;
        }
        FORS (i, startPos, size) {
            buffer_.push_back(data[i]);
        }
    }

    // 尝试解包
    tryUnpack();
}

size_t PacketProcessor::getDataPos() {
    assert(!buffer_.empty());
    assert(buffer_.size() >= dataLen_);
    return HEADER_LEN + LEN_BYTES;
}

/**
 * @return 数据净长度 不包括头、长度、校验等
 */
size_t PacketProcessor::getDataLength() {
    assert(!buffer_.empty());
    assert(buffer_.size() >= dataLen_);
    return dataLen_ - CHECK_LEN;  // 减去校验
}

uint8_t* PacketProcessor::getPayloadPtr() {
    assert(!buffer_.empty());
    assert(buffer_.size() >= dataLen_);
    return buffer_.data() + getDataPos();
}

bool PacketProcessor::findHeader() {
    if (findHeader_) return true;

    FOR (i, buffer_.size()) {
        if (buffer_[i] == H_1) {
            if (i + 1 < buffer_.size()) {
                if (buffer_[i + 1] == H_2) {
                    if (i != 0) {
                        buffer_.assign(buffer_.cbegin() + i, buffer_.cend());
                    }
                    findHeader_ = true;
                    return true;
                }
            } else {
                buffer_.assign(buffer_.cend() - 1, buffer_.cend());
                return false;
            }
        }
    }

    return false;
}

void PacketProcessor::tryUnpack() {
    if (not findHeader()) return;

    // 等足够LEN_BYTES字节时开始计算长度
    if (buffer_.size() < HEADER_LEN + LEN_BYTES) return;
    if (dataLen_ == 0) {
        uint32_t size = 0;
        const int LEN_BYTES_WITHOUT_CRC = LEN_BYTES - LEN_CRC_B;
        FOR (i, LEN_BYTES_WITHOUT_CRC) {
            size += buffer_[HEADER_LEN + i] << (LEN_BYTES_WITHOUT_CRC - i - 1) * 8;
        }

        if (size > maxBufferSize_) {
            LOGW("size too big, or data error, restart!");
            restart(HEADER_LEN);
            return;
        }

        uint16_t expectCrc = 0;
        FOR (i, LEN_CRC_B) {
            expectCrc += buffer_[HEADER_LEN + LEN_BYTES_WITHOUT_CRC + i] << (LEN_CRC_B - i - 1) * 8;
        }
        uint16_t sizeCrc = calCrc<uint32_t>(size);
        LOGV("length crc: 0x%02X  0x%02X", sizeCrc, expectCrc);

        if (sizeCrc != expectCrc) {
            LOGE("length crc error: 0x%02X != 0x%02X", sizeCrc, expectCrc);
            restart(LEN_BYTES);
            return;
        }
        dataLen_ = size;
        LOGD("headerLen_=%zu", dataLen_);
    }

    // 判断长度是否足够
    if (buffer_.size() >= HEADER_LEN + LEN_BYTES + dataLen_) {
        LOGV("buffer_.size()=%zu", buffer_.size());
        if (checkCrc()) {
            if (onPacketHandle_) {
                onPacketHandle_(getPayloadPtr(), getDataLength());
            }
            LOGV("now buffer_.size()=%zu", buffer_.size());
            restart(getNextPacketPos());
        } else {
            // 重新从buffer找 防止遗漏
            restart(HEADER_LEN);
        }
    }
}

bool PacketProcessor::checkCrc() {
    uint8_t* buffer = buffer_.data();

    uint8_t* dataPos = buffer + getDataPos();
    uint32_t dataSize = getDataLength();
    uint8_t* crcPos = dataPos + dataSize;

    uint16_t expectCrc = useCrc_ ? crc_16(dataPos, dataSize) : 0xAA55;
    uint16_t dataCrc = 0;
    FOR (i, CHECK_LEN) {
        dataCrc += crcPos[i] << 8u * (CHECK_LEN - 1 - i);
    }
    bool ret = dataCrc == expectCrc;
    if (not ret) {
        LOGE("data crc error: 0x%02X != 0x%02X", dataCrc, expectCrc);
    }
    return dataCrc == expectCrc;
}

size_t PacketProcessor::getNextPacketPos() {
    return getDataPos() + dataLen_;
}

void PacketProcessor::restart(uint32_t pos) {
    assert(buffer_.size() >= pos);

    buffer_.assign(buffer_.cbegin() + pos, buffer_.cend());

    findHeader_ = false;
    dataLen_ = 0;

    // 每次解包成功后 要继续尝试解包 因为缓冲可能包含多个包
    tryUnpack();
}
