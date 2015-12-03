#pragma once

#include "chat_session.h"
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <set>

typedef std::shared_ptr<chat_session> chat_session_ptr;

class chat_room {
public:
    chat_room(tcp::socket &&socket, tcp::acceptor &&acceptor)
        : socket_(std::move(socket)), acceptor_(std::move(acceptor)) {
        do_accept();
    }

    void leave(chat_session_ptr participant) {
        participants_.erase(participant);
    }

    void deliver(const std::string &msg) {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto participant : participants_)
            participant->push(msg);
    }

private:
    void join(chat_session_ptr participant) {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
            participant->push(msg);
    }

    void do_accept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec)
                join(std::make_shared<chat_session>(
                    std::move(socket_), std::bind(&chat_room::deliver, this,
                                                  std::placeholders::_1)));
            do_accept();
        });
    }

    std::set<chat_session_ptr> participants_;
    std::deque<std::string> recent_msgs_;
    tcp::socket socket_;
    tcp::acceptor acceptor_;
    const static int max_recent_msgs = 100;
};
