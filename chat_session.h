#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <queue>
#include <string>

using boost::asio::ip::tcp;

typedef std::function<void(const std::string &)> pulled_message_handler;

class chat_session {
public:
    chat_session(tcp::socket &&socket, pulled_message_handler handler)
        : socket_(std::move(socket)), pulled_message_handler_(handler) {
        do_pull();
    }

    void push(const std::string &msg) {
        push_queue_.push(msg + '\0');
        if (!push_running_) do_push();
    }

private:
    void do_pull() {
        boost::asio::async_read_until(
            socket_, pull_buf_, '\0',
            [this](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    read_pulled_message();
                    handle_pulled_message();
                    do_pull();
                }
            });
    }

    void do_push() {
        push_running_ = true;
        boost::asio::async_write(
            socket_, boost::asio::buffer(push_queue_.front()),
            [this](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    push_queue_.pop();
                    if (push_queue_.empty())
                        push_running_ = false;
                    else
                        do_push();
                }
            });
    }

    void read_pulled_message() {
        std::istream is(&pull_buf_);
        std::getline(is, pulled_message_, '\0');
    }

    void handle_pulled_message() {
        if (pulled_message_handler_) pulled_message_handler_(pulled_message_);
    }

private:
    tcp::socket socket_;
    bool push_running_ = false;
    boost::asio::streambuf pull_buf_;
    std::queue<std::string> push_queue_;
    std::string pulled_message_;
    pulled_message_handler pulled_message_handler_;
};
