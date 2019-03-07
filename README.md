# Yet - rtmp / http-flv 服务器

[English readme click me](./README.EN.md)

2019年写的一个rtmp服务器。欢迎star fork watch，issue提bug和需求。目标是实现一个高性能、可读可维护、模块化的流媒体服务器。

### 大体已完成

* rtmp pub: 推rtmp流至`yet`
* rtmp sub: 从`yet`拉取rtmp流
* http-flv sub: 从`yet`拉取http-flv流
* rtmp pull 回源: 当客户端从`yet`拉流而该流不存在时，从其他服务器拉取rtmp流
* rtmp push 转推: 当客户端推rtmp流至`yet`时，将流转推至其他服务器
* gop缓存

### 下一步开发路线图

~

### 配置文件说明

```
{
  "rtmp_server_ip": "0.0.0.0",        // rtmp推流和拉流监听ip
  "rtmp_server_port": 1935,           // rtmp推流和拉流监听端口
  "http_flv_server_ip": "0.0.0.0",    // http-flv拉流监听ip
  "http_flv_server_port": 8080,       // http-flv拉流监听端口
  "rtmp_pull_host": "www.google.com", // rtmp回源。如果这个字段存在，那么当所拉的流不存在时，yet会从这个配置域名回源（拉取rtmp流）
  "rtmp_push_host": "www.google.com"  // rtmp转推。如果这个字段存在，那么当有推流时，yet会转推一份流去这个配置域名
}
```

### 依赖

所有依赖的第三方库都为头文件实现，它们不需要单独配置或编译，而且已经源码包含在 yet 中，直接可以使用。
所以简单来说，你只需要一个支持c++11的编译器，并且安装好cmake就可以完成yet的编译。

* asio
* spdlog
* nlohmann/json

### 编译

#### cmake

```
# 先进入yet工程的根目录
$./build.sh
```

#### xcode

```
# 先进入yet工程的根目录
$./gen_xcode.sh
# 脚本执行完成后会在根目录下生成xcode目录，使用xcode打开xcode/yet.xcodeproj即可
```

#### 启动方法

```
$./output/bin/yet ./conf/yet.json.conf
# 成功启动后，在1935端口进行rtmp监听，在8080端口进行http-flv播放的监听，具体参见配置文件的配置
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
