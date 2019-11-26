# PacketProcessor

[![Build Status](https://github.com/shuai132/PacketProcessor/workflows/build/badge.svg)](https://github.com/shuai132/PacketProcessor/actions?workflow=build)

实现数据打包和解包，用于解决数据传输中的粘包等问题。

可配置对数据crc校验，方便在串口设备的实际应用。

## Requirements

* C++11
* CMake 3.1

## Features

* support crc16 checksum

## Usage

[example/packet_example.cpp](example/packet_example.cpp)
