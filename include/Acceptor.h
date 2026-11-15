#pragma once
#include<stdio.h>
#include<errno.h>
#include<cstdint>
#include<memory>
#include<iostream>
#include<functional>
#include<fcntl.h>

class EventLoop;
class Channel;
class Logger;

class Acceptor {
public:
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;

    Acceptor(EventLoop* loop, Logger* Logger, const uint16_t& port, const char IP[]);

    void SetConnectionCallBack(std::function<void(int)>cb);
private:
    void AcceptClient();
private:
    EventLoop* main_loop;
    Logger* logger;
    std::unique_ptr<Channel> listen_channel;
    std::function<void(int)>conn_cb;
};