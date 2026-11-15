#include"EventLoop.h"
#include"Channel.h"
#include"Connection.h"
#include"Logger.h"

EventLoop::EventLoop() :epfd_(epoll_create1(0)),
ev_channel_(std::make_unique<Channel>(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK), this)) {
    if (epfd_ == -1) {
        perror("epoll_create1");
    }

    ev_channel_->SetReadCallBack([this] {
        uint64_t zero;
        ssize_t n = read(ev_channel_->fd(), &zero, sizeof(zero));
        (void)n;
        });
    ev_channel_->EnableReading();
}

EventLoop::~EventLoop() {
    end_ = true;
    EnableEventFd();//使epoll_wait不再阻塞
}
//将fd添加至epoll
void EventLoop::AddChannel(Channel* Ch) {
    epoll_event ev;
    ev.data.ptr = Ch;
    ev.events = Ch->events();
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, Ch->fd(), &ev) == -1) {
        perror("epoll_ctl");
        if (logger_) {
            logger_->Log(LEVEL::ERROR, "EventLoop: epoll_ctl ADD failed, fd: {}, errno: {}", Ch->fd(), strerror(errno));
        }
    }
}

void EventLoop::AddConnection(std::unique_ptr<Connection> conn) {
    if (logger_) {
        logger_->Log(LEVEL::INFO, "EventLoop: connection added, fd: {}", conn->channel()->fd());
    }
    Connections_.emplace(conn->channel()->fd(), std::move(conn));
}

void EventLoop::DeleteConnection(int fd) {
    connectionsBin.emplace_back(fd);
}

void EventLoop::RunInLoop(std::function<void()> f) {
    if (std::this_thread::get_id() == thread_id_) {
        f();
    }
    else {
        AddTask(f);
    }
}

void EventLoop::loop() {
    thread_id_ = std::this_thread::get_id();
    if (logger_) {
        logger_->Log(LEVEL::INFO, "EventLoop: loop started, tid: {}",
            static_cast<size_t>(std::hash<std::thread::id>{}(thread_id_)));
    }

    epoll_event events_[1024];
    while (!end_) {
        int num = epoll_wait(epfd_, events_, 1024, -1);
        if (num == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            if (logger_) {
                logger_->Log(LEVEL::ERROR, "EventLoop: epoll_wait failed, errno: {}", strerror(errno));
            }
            break;
        }
        for (int i = 0;i < num;i++) {
            Channel* ch = static_cast<Channel*>(events_[i].data.ptr);
            ch->HandleEvent();
        }

        std::deque<int> bin;
        bin.swap(connectionsBin);
        for (auto& it : bin) {
            Connections_.erase(it);
        }

        HandleTasks();
    }

    if (logger_) {
        logger_->Log(LEVEL::INFO, "EventLoop: loop stopped, tid: {}",
            static_cast<size_t>(std::hash<std::thread::id>{}(thread_id_)));
    }
}

void EventLoop::HandleTasks() {
    std::deque<std::function<void()>>tasks;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks.swap(tasks_);
    }
    if (logger_ && tasks.size() > 256) {
        logger_->Log(LEVEL::WARN, "EventLoop: task backlog is large, size: {}", tasks.size());
    }
    for (auto& it : tasks) {
        it();
    }
}

void EventLoop::EnableEventFd() {
    uint64_t one = 1;
    ssize_t n = write(ev_channel_->fd(), &one, sizeof(one));
    (void)n;
}

void EventLoop::AddTask(std::function<void()> f) {
    size_t backlog = 0;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        tasks_.emplace_back(std::move(f));
        backlog = tasks_.size();
    }
    if (logger_ && backlog > 1024) {
        logger_->Log(LEVEL::WARN, "EventLoop: task queue is growing, size: {}", backlog);
    }
    EnableEventFd();
}