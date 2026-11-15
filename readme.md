# Reactor TCP Server

一个基于 Linux `epoll` 和 C++17 的多线程 TCP Reactor 示例项目，采用主从事件循环结构，并提供命令行客户端与异步日志模块。

## 特性

- **主从 Reactor**：主事件循环负责接收连接，多个子事件循环分担连接 I/O。
- **非阻塞 I/O**：基于 `epoll` 处理 socket 事件。
- **长度前缀协议**：使用 4 字节网络字节序长度头解析 TCP 消息。
- **异步日志**：日志通过线程安全队列交由独立写线程写入文件，支持 `DEBUG`、`INFO`、`WARN`、`ERROR` 级别。
- **命令行客户端**：连接服务端并发送文本消息。

服务端当前接收消息并记录连接、错误等运行信息；不会回显或广播消息，也不会将收到的正文写入日志。

## 架构

```text
Client ──TCP──> Acceptor ──> Main EventLoop
                                  │
                          round-robin dispatch
                           ┌──────┴──────┐
                       Sub EventLoop  Sub EventLoop ...
                           │
                       Connection

Server components ──> Logger queue ──> Log writer thread ──> Log file
```

主要组件：

| 组件 | 职责 |
| --- | --- |
| `Acceptor` | 监听地址并接受新连接 |
| `EventLoop` / `Channel` | `epoll` 事件循环、fd 和回调管理 |
| `Connection` / `Buffer` | 连接生命周期、缓冲与消息帧解析 |
| `TcpServer` | 创建主/子事件循环并分配连接 |
| `Logger` / `Formatter` | 格式化日志，并通过后台线程异步写入 |
| `Client` | 连接服务端并发送文本消息 |

## 构建

依赖：Linux、支持 C++17 的 `g++`、GNU Make、pthread 和 gperftools（`libtcmalloc`）。

```bash
make
```

构建后生成 `Server` 和 `Client`。清理构建产物：

```bash
make clean
```

## 快速开始

在终端 1 启动服务端：

```bash
./Server
```

默认监听 `127.0.0.1:9999`，并创建 4 个子事件循环。

在终端 2 启动客户端：

```bash
./Client
```

连接后输入文本并按回车即可发送。服务端会解析消息帧，并记录相关运行日志。

## 消息格式

每条消息由 4 字节无符号网络字节序长度和对应长度的正文组成。长度字段不包含自身；服务端当前接受的正文长度为 `1–4096` 字节。

## 异步日志

`Logger` 将日志消息放入受互斥锁保护的队列，由专用写线程批量取出并写入文件。默认最低级别为 `INFO`；低于最低级别的日志会被忽略。服务端在 `TcpServer.cpp` 中配置日志文件路径，目前默认为：

```text
/home/kiuwu95/socket/reactor/Log.log
```

日志文件以追加模式打开。部署到其他环境时，请先将此路径改为可写且适合目标环境的位置。

## 项目结构

```text
.
├── include/           # 组件头文件
├── Acceptor.cpp       # 接收 TCP 连接
├── Buffer.cpp         # 网络缓冲区
├── Channel.cpp        # fd 与事件回调
├── Client.cpp         # 客户端实现
├── Clt.cpp            # 客户端入口
├── Connection.cpp     # 服务端连接处理
├── EventLoop.cpp      # epoll 事件循环
├── Logger.cpp         # 异步日志实现
├── Svr.cpp            # 服务端入口
├── TcpServer.cpp      # 服务端主逻辑
├── Makefile
└── CHANGELOG.md       # 版本记录与性能测试
```

## 当前范围

本项目聚焦于 Reactor、非阻塞 I/O、连接管理和异步日志的实现与学习，不是生产级服务器框架。当前不包含消息广播、业务路由、日志滚动、配置文件或连接超时管理。
