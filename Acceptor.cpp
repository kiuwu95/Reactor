#include"Acceptor.h"
#include"EventLoop.h"
#include"Channel.h"
#include"Logger.h"

Acceptor::Acceptor(EventLoop* loop, Logger* Logger, const uint16_t& port, const char IP[]) :
    main_loop(loop), logger(Logger) {

    logger->Log(LEVEL::INFO, "Acceptor: initializing, IP: {}, port: {}", IP, port);

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd == -1) {
        perror("socket");
        logger->Log(LEVEL::ERROR, "Acceptor: socket() failed, errno: {}", strerror(errno));

        return;
    }
    fcntl(lfd, F_SETFL, fcntl(lfd, F_GETFL, 0) | O_NONBLOCK);

    sockaddr_in saddr;
    saddr.sin_port = htons(port);
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, IP, &saddr.sin_addr.s_addr);
    if (bind(lfd, (sockaddr*)&saddr, sizeof(saddr)) == -1) {
        perror("bind");
        logger->Log(LEVEL::ERROR, "Acceptor: bind failed on {}:{}, errno: {}", IP, port, strerror(errno));

        return;
    }
    if (listen(lfd, SOMAXCONN) == -1) {
        perror("listen");
        logger->Log(LEVEL::ERROR, "Acceptor: listen failed, errno: {}", strerror(errno));

        return;
    }

    listen_channel = std::make_unique<Channel>(lfd, loop);

    listen_channel->SetReadCallBack([this] {AcceptClient();});

    listen_channel->EnableReading();

    logger->Log(LEVEL::INFO, "Acceptor: start listening on {}:{}", IP, port);
}
void Acceptor::SetConnectionCallBack(std::function<void(int)> cb) {
    conn_cb = std::move(cb);
}
void Acceptor::AcceptClient() {
    while (1) {
        sockaddr_in caddr;
        socklen_t addr_len = sizeof(caddr);
        bzero(&caddr, sizeof(caddr));
        int cfd = accept(listen_channel->fd(), (sockaddr*)&caddr, &addr_len);
        if (cfd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            perror("accept");
            logger->Log(LEVEL::ERROR, "Acceptor: accept failed, errno: {}", strerror(errno));

            break;
        }

        char IP[32] = { 0 };
        inet_ntop(AF_INET, &caddr.sin_addr.s_addr, IP, sizeof(IP));
        logger->Log(LEVEL::INFO, "Acceptor: new client, IP: {}, port: {}, fd: {}",
            IP, ntohs(caddr.sin_port), cfd);

        conn_cb(cfd);
    }
}