#pragma once

#include "session.h"
#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <set>

using SessionPtr = std::shared_ptr<Session>;

class ChatRoom {
public:
    ChatRoom(tcp::socket &&socket, tcp::acceptor &&acceptor)
        : socket_(std::move(socket)), acceptor_(std::move(acceptor)) {
        DoAccept();
    }

    void Leave(SessionPtr session) { sessions_.erase(session); }

    void Deliver(const std::string &msg) {
        recentMsgs_.push_back(msg);
        while (recentMsgs_.size() > RecentMsgsMaxCount)
            recentMsgs_.pop_front();

        for (auto session : sessions_)
            session->Push(msg);
    }

    void Join(SessionPtr session) {
        sessions_.insert(session);
        for (auto msg : recentMsgs_)
            session->Push(msg);
    }

private:
    void DoAccept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec)
                Join(std::make_shared<Session>(
                    std::move(socket_), std::bind(&ChatRoom::Deliver, this,
                                                  std::placeholders::_1)));
            DoAccept();
        });
    }

    const static int RecentMsgsMaxCount = 100;
    std::set<SessionPtr> sessions_;
    std::deque<std::string> recentMsgs_;
    tcp::socket socket_;
    tcp::acceptor acceptor_;
};
