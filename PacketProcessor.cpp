#include "PacketProcessor.h"

#include <cassert>
#include <utility>

#include "crc/checksum.h"

// #define PacketProcessor_LOG_SHOW_VERBOSE
#include "log.h"

#define FORS(i, s, n) for (typename std::remove_reference<typename std::remove_const<decltype(n)>::type>::type i = s; i < n; i++)
#define FOR(i, n) FORS(i, 0, n)

/**
 * 求一个值类型的 按大端序的 crc校验
 */
template <typename T>
static uint16_t calCrc(T param) {
  constexpr auto size = sizeof(param);
  uint8_t tmp[size];
  FOR(i, size) {
    tmp[i] = param >> 8u * (size - i - 1);
  }
  return crc_16(tmp, size);
}

PacketProcessor::PacketProcessor(OnPacketHandle handle, bool useCrc) : onPacketHandle_(std::move(handle)), useCrc_(useCrc) {}

void PacketProcessor::setOnPacketHandle(const OnPacketHandle& handle) {
  onPacketHandle_ = handle;
}

void PacketProcessor::setUseCrc(bool enable) {
  useCrc_ = enable;
}

void PacketProcessor::setMaxBufferSize(uint32_t size) {
  assert(size > 0);
  maxBufferSize_ = size + ALL_HEADER_LEN;
  clearBuffer();
}

void PacketProcessor::clearBuffer() {
  decltype(buffer_) tmp;
  tmp.swap(buffer_);
  findHeader_ = false;
  dataSize_ = 0;
}

std::string PacketProcessor::pack(const void* data, uint32_t size) const {
  uint32_t payloadSize = size + ALL_HEADER_LEN;
  std::string payload;
  payload.reserve(payloadSize);
  packForeach(data, size, [&](uint8_t* data, size_t size) {
    while (size--) {
      payload.push_back((char)*data++);
    }
  });
  return payload;
}

std::string PacketProcessor::pack(const std::string& data) const {
  return pack(data.data(), data.length());
}

/**
 * 约定形式: 包头2字节(0x5AA5)+数据净长度4字节(大端序)+长度校验2字节(长度CRC16)+数据+校验2字节(数据CRC16/长度CRC16的按位取反)
 * @param data
 * @param size
 */
void PacketProcessor::packForeach(const void* data, uint32_t size, const std::function<void(uint8_t* data, size_t size)>& handle) const {
  uint8_t tmp[4];

  tmp[0] = H_1;
  tmp[1] = H_2;
  handle(tmp, 2);

  uint32_t dataSize = size;
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

  uint16_t crcSum = useCrc_ ? crc_16((uint8_t*)data, size) : ~calCrc(size);
  tmp[0] = (crcSum & 0xff00) >> 8 * 1;
  tmp[1] = (crcSum & 0x00ff) >> 8 * 0;
  handle(tmp, 2);
}

void PacketProcessor::feed(const void* d, size_t size) {
  const uint8_t* data = (uint8_t*)d;
  if (size == 0) return;
  PacketProcessor_LOGV("feed: %u", size);

  // 缓存数据(当遇到包头后才开始缓存)
  size_t startPos = 0;
  if (buffer_.empty()) {
  START_HEADER:
    FOR(i, size) {
      if (data[i] == H_1) {
        if (i + 1 < size) {
          if (data[i + 1] == H_2) {
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
    if (data[0] == H_2)
      goto START_BUFFER;
    else {
      buffer_.clear();
      buffer_.shrink_to_fit();
      goto START_HEADER;
    }
  } else {
  START_BUFFER:
    const auto needSize = buffer_.size() + size - startPos;
    if (needSize > maxBufferSize_) {
      PacketProcessor_LOGW("size too big, need: %zu, max: %zu", needSize, (size_t)maxBufferSize_);
      clearBuffer();
      return;
    }
    buffer_.insert(buffer_.end(), data + startPos, data + size - startPos);
  }

  // 尝试解包
  tryUnpack();
}

size_t PacketProcessor::getDataPos() {
  assert(buffer_.size() >= dataSize_ + ALL_HEADER_LEN);
  return HEADER_LEN + LEN_BYTES;
}

/**
 * @return 数据净长度 不包括头、长度、校验等
 */
size_t PacketProcessor::getDataSize() {
  assert(buffer_.size() >= dataSize_ + ALL_HEADER_LEN);
  return dataSize_;
}

uint8_t* PacketProcessor::getPayloadPtr() {
  assert(!buffer_.empty());
  assert(buffer_.size() >= dataSize_ + ALL_HEADER_LEN);
  return buffer_.data() + getDataPos();
}

bool PacketProcessor::findHeader() {
  if (findHeader_) return true;

  FOR(i, buffer_.size()) {
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
  if (dataSize_ == 0) {
    uint32_t size = 0;
    const unsigned int LEN_BYTES_WITHOUT_CRC = LEN_BYTES - LEN_CRC_B;
    FOR(i, LEN_BYTES_WITHOUT_CRC) {
      size += buffer_[HEADER_LEN + i] << (LEN_BYTES_WITHOUT_CRC - i - 1) * 8;
    }

    if (size == 0) {
      PacketProcessor_LOGE("size can not be zero!");
      restart(HEADER_LEN);
      return;
    }

    if (size > maxBufferSize_) {
      PacketProcessor_LOGW("size too big, or data error, restart!");
      restart(HEADER_LEN);
      return;
    }

    uint16_t expectSizeCrc = 0;
    FOR(i, LEN_CRC_B) {
      expectSizeCrc += buffer_[HEADER_LEN + LEN_BYTES_WITHOUT_CRC + i] << (LEN_CRC_B - i - 1) * 8;
    }
    uint16_t sizeCrc = calCrc<uint32_t>(size);
    PacketProcessor_LOGV("length crc: 0x%02X  0x%02X", sizeCrc, expectSizeCrc);

    if (sizeCrc != expectSizeCrc) {
      PacketProcessor_LOGE("size crc error: 0x%02X != 0x%02X", sizeCrc, expectSizeCrc);
      restart(HEADER_LEN);
      return;
    }
    dataSize_ = size;
    PacketProcessor_LOGD("headerLen_=%zu", dataSize_);
  }

  // 判断长度是否足够
  if (buffer_.size() >= dataSize_ + ALL_HEADER_LEN) {
    PacketProcessor_LOGV("buffer_.size()=%zu", buffer_.size());
    if (checkCrc()) {
      if (onPacketHandle_) {
        onPacketHandle_(getPayloadPtr(), getDataSize());
      }
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
  uint32_t dataSize = getDataSize();
  uint8_t* crcPos = dataPos + dataSize;

  uint16_t expectDataCrc = useCrc_ ? crc_16(dataPos, dataSize) : ~calCrc(dataSize);
  uint16_t dataCrc = 0;
  FOR(i, CHECK_LEN) {
    dataCrc += (decltype(dataCrc))(crcPos[i]) << 8u * (CHECK_LEN - 1 - i);
  }
  bool ret = dataCrc == expectDataCrc;
  if (not ret) {
    PacketProcessor_LOGE("data crc error: 0x%02X != 0x%02X", dataCrc, expectDataCrc);
  }
  return dataCrc == expectDataCrc;
}

size_t PacketProcessor::getNextPacketPos() {
  return getDataPos() + dataSize_ + CHECK_LEN;
}

void PacketProcessor::restart(uint32_t pos) {
  PacketProcessor_LOGV("restart: pos=%u,  buffer_.size()=%zu", pos, buffer_.size());
  assert(buffer_.size() >= pos);

  buffer_.assign(buffer_.cbegin() + pos, buffer_.cend());

  findHeader_ = false;
  dataSize_ = 0;

  // 每次解包成功后 要继续尝试解包 因为缓冲可能包含多个包
  tryUnpack();
}
