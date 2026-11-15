#include"TcpServer.h"
#include"EventLoop.h"
#include"Channel.h"
#include"Acceptor.h"
#include"Connection.h"
#include"Logger.h"

TcpServer::TcpServer(const char IP[], const uint16_t& port, const size_t& loops_num) :
    main_loop(std::make_unique<EventLoop>()),
    logger_(std::make_unique<Logger>("/home/kiuwu95/socket/reactor/Log.log")) {

    main_loop->SetLogger(logger_.get());

    logger_->Log(LEVEL::INFO, "Lauching server...");
    logger_->Log(LEVEL::INFO, "Server's IP: {}, Port: {}, Loops_num: {}", IP, port, loops_num);

    acceptor_ = std::make_unique<Acceptor>(main_loop.get(), logger_.get(), port, IP);
    acceptor_->SetConnectionCallBack([this](int fd) {AcceptorConnectionCallBack(fd);});

    for (size_t i = 0;i < loops_num;i++) {
        std::unique_ptr<EventLoop>sub_loop = std::make_unique<EventLoop>();
        sub_loop->SetLogger(logger_.get());
        sub_loops.emplace_back(std::move(sub_loop));
    }

    logger_->Log(LEVEL::INFO, "Lauch server successfully!");
}
TcpServer::~TcpServer() {
    logger_->Log(LEVEL::INFO, "Closing server...");

    for (auto& it : threads_) {
        if (it.joinable()) { it.join(); }
    }

    logger_->Log(LEVEL::INFO, "Close server successfully!");
}

void TcpServer::start() {
    logger_->Log(LEVEL::INFO, "Starting server, sub loop num: {}", sub_loops.size());

    for (auto& it : sub_loops) {
        threads_.emplace_back([ptr = it.get()] {
            ptr->loop();
            });
    }
    threads_.emplace_back([this] {
        main_loop->loop();
        });

    logger_->Log(LEVEL::INFO, "Start server successfully!");
}

void TcpServer::AcceptorConnectionCallBack(int fd) {
    size_t index = loops_index++ % sub_loops.size();
    EventLoop* loop = sub_loops[index].get();

    logger_->Log(LEVEL::INFO, "TcpServer: new connection, fd: {}, assigned to loop {}", fd, index);

    loop->RunInLoop([fd, loop, this] {
        std::unique_ptr<Connection> conn = std::make_unique<Connection>(fd, loop);
        Connection* ptr = conn.get();

        conn->channel()->SetReadCallBack([fd, ptr, this] {
            std::string msg;
            while (!(msg = ptr->RecvMsg()).empty()) {
                logger_->Log(LEVEL::DEBUG, "TcpServer: recv message, fd: {}, size: {}", fd, msg.size());
            }
            });

        loop->AddConnection(std::move(conn));
        ptr->channel()->EnableReading();
        });

}