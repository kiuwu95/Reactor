#include"Connection.h"
#include"Channel.h"
#include"EventLoop.h"
#include"Logger.h"

Connection::Connection(const int& fd, EventLoop* loop) :
    read_buff_(std::make_unique<Buffer>()),
    write_buff_(std::make_unique<Buffer>()),
    channel_(std::make_unique<Channel>(fd, loop)) {
    logger_ = loop->logger();
    if (logger_) {
        logger_->Log(LEVEL::INFO, "Connection: established, fd: {}", fd);
    }
}

const std::string Connection::RecvMsg() {
    while (1) {
        while (read_buff_->readableBytes() >= 4) {
            uint32_t MSGSIZE;
            memcpy(&MSGSIZE, read_buff_->peek(), 4);
            MSGSIZE = ntohl(MSGSIZE);

            if (MSGSIZE == 0 || MSGSIZE > 4096) {
                if (logger_) {
                    logger_->Log(LEVEL::WARN, "Connection: invalid frame length {}, fd: {}, drop header",
                        MSGSIZE, channel_->fd());
                }
                read_buff_->hread(4);
                return "";
            }
            if (MSGSIZE + 4 > read_buff_->readableBytes()) {
                break;
            }

            read_buff_->hread(4);

            rmsg.assign(read_buff_->peek(), MSGSIZE);
            read_buff_->hread(MSGSIZE);

            return rmsg;
        }

        ssize_t n = read_buff_->readn(channel_->fd());
        if (n > 0) {
            continue;
        }

        if (n == 0) {
            if (logger_) {
                logger_->Log(LEVEL::INFO, "Connection: closed by peer, fd: {}", channel_->fd());
            }
            std::cout << "Client " << channel_->fd() << " has disconnected..." << std::endl;
            this->DeleteConnection();
            return "";
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return "";
        }
        if (errno == EINTR) {
            continue;
        }
        perror("recv");
        if (logger_) {
            logger_->Log(LEVEL::WARN, "Connection: recv failed, fd: {}, errno: {}",
                channel_->fd(), strerror(errno));
        }
        return "";
    }
}

void Connection::SendMsg(std::string_view Msg) {
    uint32_t MSGSIZE = Msg.size();
    MSGSIZE = htonl(MSGSIZE);
    write_buff_->Append((char*)&MSGSIZE, 4);
    write_buff_->Append(Msg.data(), Msg.size());

    if (write_buff_->writen(channel_->fd()) == -1) {
        perror("writen");
        if (logger_) {
            logger_->Log(LEVEL::WARN, "Connection: send failed, fd: {}, errno: {}",
                channel_->fd(), strerror(errno));
        }
    }
}



void Connection::DeleteConnection() {
    channel_->loop()->DeleteConnection(channel_->fd());
}

Channel* Connection::channel() const {
    return channel_.get();
}