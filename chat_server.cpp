#include <boost/asio.hpp>
#include <deque>
#include <cstdlib>
#include <queue>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>

using boost::asio::ip::tcp;

class chat_participant {
public:
    virtual ~chat_participant() {}
    virtual void deliver(const std::string &msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//--------------------------------------------------------------------------------

class chat_room {
public:
    void join(chat_participant_ptr participant) {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
            participant->deliver(msg);
    }

    void leave(chat_participant_ptr participant) {
        participants_.erase(participant);
    }

    void deliver(const std::string &msg) {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto participant : participants_)
            participant->deliver(msg);
    }

private:
    std::set<chat_participant_ptr> participants_;
    const static int max_recent_msgs = 100;
    std::deque<std::string> recent_msgs_;
};

//--------------------------------------------------------------------------------

class chat_session : public chat_participant,
                     public std::enable_shared_from_this<chat_session> {
public:
    chat_session(tcp::socket socket, chat_room &room)
        : socket_(std::move(socket)), room_(room) {}

    void start() {
        room_.join(shared_from_this());
        do_pull();
    }

    void deliver(const std::string &msg) {
        push_queue_.push(msg + '\0');
        if (!push_running_) do_push();
    }

private:
    void do_pull() {
        auto self(shared_from_this());
        boost::asio::async_read_until(
            socket_, pull_buf_, '\0',
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    room_.deliver(take_pulled_msg());
                    do_pull();
                } else
                    room_.leave(shared_from_this());
            });
    }

    void do_push() {
        push_running_ = true;
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(push_queue_.front()),
            [this, self](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    push_queue_.pop();
                    if (push_queue_.empty())
                        push_running_ = false;
                    else
                        do_push();
                } else
                    room_.leave(shared_from_this());
            });
    }

    std::string take_pulled_msg() {
        std::string message;
        std::istream is(&pull_buf_);
        std::getline(is, message, '\0');
        return message;
    }

private:
    tcp::socket socket_;
    chat_room &room_;
    bool push_running_ = false;
    boost::asio::streambuf pull_buf_;
    std::queue<std::string> push_queue_;
};

//--------------------------------------------------------------------------------

class chat_server {
public:
    chat_server(boost::asio::io_service &io_service,
                const tcp::endpoint &endpoint)
        : acceptor_(io_service, endpoint), socket_(io_service) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_, [this](boost::system::error_code ec) {
            if (!ec)
                std::make_shared<chat_session>(std::move(socket_), room_)
                    ->start();
            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    chat_room room_;
};

//--------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <port> [<port> ...]"
                      << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;

        std::list<chat_server> servers;
        for (int i = 0; i < argc; ++i) {
            tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
            servers.emplace_back(io_service, endpoint);
        }

        io_service.run();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
