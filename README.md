# Yet - rtmp / http-flv 服务器

[English readme click me](./README.EN.md)

都9102了，我要开始写一个 rtmp / http-flv 服务器了，哈哈。

欢迎star fork watch issue。

目标是实现一个高性能、可读可维护、模块化的流媒体服务器。

### 大体完成

* rtmp 推流
* rtmp 拉流
* http-flv 拉流

### 下一步开发路线图

* rtmp pull回源
* rtmp push转推

### 依赖

所有依赖的第三方库都为头文件实现，它们不需要单独配置或编译，而且已经源码包含在 yet 中，直接可以使用。
所以简单来说，你只需要一个支持c++11的编译器，并且安装好cmake就可以完成yet的编译。

* asio
* spdlog
* nlohmann/json

```
$./build.sh
```

### 我的环境

```
OS X EI Capitan 10.11.6
Apple LLVM version 8.0.0 (clang-800.0.42.1)

Linux xxx 2.6.32-642.15.1.el6.x86_64 #1 SMP Fri Feb 24 14:31:22 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux
gcc version 7.1.0 (GCC)
```

### 性能

和 nginx-rtmp-module / srs / crtmpserver 或其他相似类型的服务器做比较

### 其他
