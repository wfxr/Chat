#include <boost/asio.hpp>
#include <iostream>
#include <queue>
#include <string>

using boost::asio::ip::tcp;

class chat_session {
public:
    chat_session(tcp::socket socket) : socket_(std::move(socket)) {}

    void push(const std::string &msg) {
        push_queue_.push(msg + '\0');
        if (!push_running_) do_push();
    }

    void start() { do_pull(); }

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

    void handle_pulled_message() { std::cout << pulled_message_ << std::endl; }

private:
    tcp::socket socket_;
    bool push_running_ = false;
    boost::asio::streambuf pull_buf_;
    std::queue<std::string> push_queue_;
    std::string pulled_message_;
};
