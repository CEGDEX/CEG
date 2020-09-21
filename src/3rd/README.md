
# Third Party Library

## Introduction
The third party library implements various functions in an open source manner, and CEG integrates and calls these libraries. Using a three-party library has the following advantages:
- Rich functions
- Open source code
- Reduce repeated development
- Low cost to maintain

## Module Structure

Name | Directory | Function
|:--- | --- | ---
| `asio` | [asio](./asio) | Originated from the `boost::asio` module, it provides a cross-platform network underlying asynchronous IO library.
| `bzip2` | [bzip2-1.0.6](./bzip2-1.0.6) | It implements efficient compression algorithms.
| `curl` | [curl](./curl) | It implements a variety of protocols such as HTTP, and supports uploading and downloading multiple applications.
| `ed25519` | [ed25519-donna](./ed25519-donna) | It implements a high performance digital signature algorithm.
| `gtest` | [gtest](./gtest) | Google's C++ unit testing framework, abbreviated for Google Test.
| `http` | [http](./http) | Originated from the `boost.asio` module, it provides the `HTTP` client and server basic functionality.
| `jsoncpp` | [jsoncpp](./jsoncpp) | It processes the C++ library for JSON text.
| `scrypt` | [libscrypt](./libscrypt) | HASH algorithm, long time, accounted for memory, violent attack is difficult.
| `openssl` | [openssl](./openssl) | It provides symmetric encryption algorithms (AES, DES, etc.), asymmetric encryption algorithms (AES, DES, etc.) and digest algorithms (MD5, SHA1, etc.).
| `pcre` | [pcre-8.39](./pcre-8.39) | It provides the ability to use regular expressions in the C/C++ language.
| `protobuf` | [protobuf](./protobuf) | An efficient data description language developed by Google Inc., generally used in storage, communication protocols and other scenarios.
| `rocksdb` | [rocksdb](./rocksdb) | An embeddable, persistent key-value database developed by Facebook.
| `sqlite` | [sqlite](./sqlite) | Lightweight relational database, which consumes low resources and is generally embedded in product use.
| `websocketpp` | [websocketpp](./websocketpp) | WebSocket client/server library for C++.
| `zlib` | [zlib-1.2.8](./zlib-1.2.8) | A library that provides data compression, supports the variant algorithm of LZ77, and algorithm of DEFLATE.
| `asio` | [asio](./asio) | Originated fro the `boost::asio` module, it supports the underlying asynchronous IO functionality of the cross-platform network.
| `v8` | [v8](https://github.com/v8/v8) | High performance JaScript engine developed by Google.
