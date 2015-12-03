#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <queue>
#include <string>

using boost::asio::ip::tcp;

typedef std::function<void(const std::string &)> PulledMsgHandler;

class Session {
public:
    Session(tcp::socket &&socket, PulledMsgHandler handler)
        : socket_(std::move(socket)), pulledMsgHandler_(handler) {
        DoPull();
    }

    void Push(const std::string &msg) {
        pushBuf_.push(msg + '\0');
        if (!isPushing_) DoPush();
    }

private:
    void DoPull() {
        boost::asio::async_read_until(
            socket_, pullBuf_, '\0',
            [this](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    ReadPulledMsg();
                    HandlePulledMsg();
                    DoPull();
                }
            });
    }

    void DoPush() {
        isPushing_ = true;
        boost::asio::async_write(
            socket_, boost::asio::buffer(pushBuf_.front()),
            [this](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    pushBuf_.pop();
                    if (pushBuf_.empty())
                        isPushing_ = false;
                    else
                        DoPush();
                }
            });
    }

    void ReadPulledMsg() {
        std::istream is(&pullBuf_);
        std::getline(is, pulledMsg_, '\0');
    }

    void HandlePulledMsg() {
        if (pulledMsgHandler_) pulledMsgHandler_(pulledMsg_);
    }

private:
    tcp::socket socket_;
    bool isPushing_ = false;
    boost::asio::streambuf pullBuf_;
    std::queue<std::string> pushBuf_;
    std::string pulledMsg_;
    PulledMsgHandler pulledMsgHandler_;
};
