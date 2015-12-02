#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <queue>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

class chat_client {
public:
    chat_client(boost::asio::io_service &io_service,
                tcp::resolver::iterator endpoint_iterator)
        : io_service_(io_service), socket_(io_service) {
        do_connect(endpoint_iterator);
    }

    // Note:发送消息不宜采用streambuf实现
    // 使用streambuf执行异步写入需要streambuf在写入期间不被外部改动，
    // 而外部如果使用循环调用push方法，可能上一次的异步写入尚未完成，
    // streambuf就已经被下一次的push操作改写
    void push(const std::string &msg) {
        io_service_.post([this, msg]() {
            push_queue_.push(msg + '\0');
            if (!push_running_) do_push();
        });
    }

    void close() {
        io_service_.post([this] { socket_.close(); });
    }

private:
    void do_connect(tcp::resolver::iterator endpoint_iterator) {
        boost::asio::async_connect(
            socket_, endpoint_iterator,
            [this](boost::system::error_code ec, tcp::resolver::iterator) {
                if (!ec) do_pull();
            });
    }

    void do_pull() {
        boost::asio::async_read_until(
            socket_, pull_buf_, '\0',
            [this](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    read_message();
                    print_message();
                    do_pull();
                } else
                    socket_.close();
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
                } else
                    socket_.close();
            });
    }

    void read_message() {
        std::istream is(&pull_buf_);
        std::getline(is, pulled_message_, '\0');
    }

    void print_message() {
        std::cout << pulled_message_ << std::endl;
    }

private:
    boost::asio::io_service &io_service_;
    tcp::socket socket_;
    bool push_running_ = false;
    boost::asio::streambuf pull_buf_;
    std::queue<std::string> push_queue_;
    std::string pulled_message_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;

        tcp::resolver resolver(io_service);
        auto endpoint_iterator = resolver.resolve({argv[1], argv[2]});
        chat_client client(io_service, endpoint_iterator);

        std::thread th([&io_service]() { io_service.run(); });

        std::string message;
        while (std::getline(std::cin, message))
            client.push(message);

        client.close();
        th.join();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
