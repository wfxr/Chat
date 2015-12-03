#include "chat_session.h"
#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <utility>

using boost::asio::ip::tcp;

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
                    std::move(socket_),
                    [this](const std::string message) { deliver(message); }));
            do_accept();
        });
    }

    std::set<chat_session_ptr> participants_;
    std::deque<std::string> recent_msgs_;
    tcp::socket socket_;
    tcp::acceptor acceptor_;
    const static int max_recent_msgs = 100;
};

//--------------------------------------------------------------------------------

class chat_server {
public:
    void create_chat_room(const std::string &port) {
        tcp::endpoint endpoint(tcp::v4(), std::stoi(port));
        tcp::socket socket(io_service_);
        tcp::acceptor acceptor(io_service_, endpoint);
        chat_rooms_.emplace_back(std::move(socket), std::move(acceptor));
    }

    void start() { io_service_.run(); }

private:
    boost::asio::io_service io_service_;
    std::list<chat_room> chat_rooms_;
};

//--------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <port> [<port> ...]"
                      << std::endl;
            return 1;
        }

        chat_server server;
        for (int i = 1; i < argc; ++i)
            server.create_chat_room(argv[i]);

        server.start();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
