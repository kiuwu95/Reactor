#pragma once
#include<sys/epoll.h>
#include<sys/eventfd.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<memory>
#include<atomic>
#include<mutex>
#include<deque>
#include<thread>
#include<functional>
#include<unordered_map>

class Channel;
class Connection;
class Logger;

class EventLoop {
public:
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(const EventLoop&) = delete;

    EventLoop();
    ~EventLoop();

    void AddChannel(Channel* Ch);
    void AddConnection(std::unique_ptr<Connection> Conn);
    void DeleteConnection(int fd);
    void RunInLoop(std::function<void()> f);

    void loop();

    int epfd() const { return epfd_; };

    void HandleTasks();
    void EnableEventFd();
    void AddTask(std::function<void()> f);

    void SetLogger(Logger* logger) { logger_ = logger; }
    Logger* logger() const { return logger_; }
private:
    int epfd_;
    std::atomic<bool> end_ = false;

    std::unordered_map<int, std::unique_ptr<Connection>> Connections_;
    std::deque<int> connectionsBin;

    std::unique_ptr<Channel> ev_channel_;
    std::deque<std::function<void()>>tasks_;
    std::mutex mtx_;
    std::thread::id thread_id_;

    Logger* logger_ = nullptr;
};