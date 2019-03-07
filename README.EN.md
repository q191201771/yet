# Yet - a rtmp / http-flv server

I'm writting a rtmp server inside in 2019. All star fork watch issue are welcome. Aim to make a high-performance, readable, maintainable, portable live-stream server.

### done almostly

* rtmp pub: publish rtmp live stream to `yet`
* rtmp sub: play rtmp live stream from `yet`
* http-flv sub: play http-flv live stream from `yet`
* rtmp pull: pull rtmp live stream from other server while client play from `yet` but stream not exist
* rtmp push: push rtmp live stream to other server while client publish rtmp live stream to `yet`

### roadmap

~

### conf file at runtime

~

### dep

All third party library are header-only, which means we don't have to config or compile them , and I've include them inside `yet`, using them directly.
Simply put, all you need to compile `yet` is a mordern c++11 compiler and cmake.

* asio
* spdlog
* nlohmann/json

### compile

#### cmake

```
# cd into yet
$./build.sh
```

#### xcode

```
# cd into yet
$./gen_xcode.sh
# this script will gen a document named xcode in yet dir, then using xcode open xcode/yet.xcodeproj.
```

### env

```
OS X EI Capitan 10.11.6
Apple LLVM version 8.0.0 (clang-800.0.42.1)

Linux xxx 2.6.32-642.15.1.el6.x86_64 #1 SMP Fri Feb 24 14:31:22 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux
gcc version 7.1.0 (GCC)
```

### benchmark

compare with  nginx-rtmp-module / srs / crtmpserver and other rtmp like server

### other

