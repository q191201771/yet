[English readme click me](./README.EN.md)

都9102了，我要开始写一个 rtmp / http-flv 服务器——yet，哈哈

欢迎star fork watch issue。

### 开发中

#### 1. http-flv 播放， http-flv 回源

使用 http-flv 播放，如果流不存在，上其他服务器（比如另一个 yet 或其他支持 http-flv 播放的服务器）

#### 2. rtmp 转发

使用 rtmp 推流， rtmp 播放，挂载它们，并进行音视频数据转发。

### 依赖

* asio
* spdlog

### 其他

![http_flv_sub_pull](./doc/http_flv_sub_pull.jpg)

![rtmp_broadcast](./doc/rtmp_broadcast.jpg)
