# PacketProcessor

[![Build Status](https://github.com/shuai132/PacketProcessor/workflows/build/badge.svg)](https://github.com/shuai132/PacketProcessor/actions?workflow=build)

实现数据打包和解包，用于解决数据传输中的粘包等问题。

包头2字节(0x5AA5)+数据净长度4字节(大端序)+长度校验2字节(长度CRC16)+数据+校验2字节(数据CRC16/长度CRC16的按位取反)

## Requirements

* C++11
* CMake 3.1

## Features

* CRC16 for data length
* CRC16 of data is option (default is data size CRC)
* Only `10 bytes` for data header and CRC
* Support `packForeach` avoid unnecessary data copy

## Usage

* simplest
```cpp
PacketProcessor processor([&](uint8_t* data, size_t size) {
    LOG("Got packet: %zu, %s", size, std::string((char*)data, size).c_str());
});
auto payload = processor.pack("hello world");
processor.feed(payload.data(), payload.size());
```

* full test  
[example/packet_example.cpp](example/packet_example.cpp)
