#include <boost/asio.hpp>
#include <cstdlib>
#include <deque>
#include <iostream>
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

    void write(const std::string &msg) {
        io_service_.post([this, msg]() {
            auto write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress) do_write();
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
                if (!ec) do_read();
            });
    }

    void do_read() {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        boost::asio::async_read_until(
            socket_, *buffer, '\0',
            [this, buffer](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    std::istream is(buffer.get());
                    std::getline(is, read_msg_, '\0');
                    std::cout << read_msg_ << std::endl;
                    do_read();
                } else {
                    socket_.close();
                }
            });
    }

    void do_write() {
        auto buffer = std::make_shared<boost::asio::streambuf>();
        std::ostream os(buffer.get());
        os << write_msgs_.front() << '\0';
        boost::asio::async_write(
            socket_, *buffer,
            [this, buffer](boost::system::error_code ec, size_t /*length*/) {
                if (!ec) {
                    write_msgs_.pop_front();
                    if (!write_msgs_.empty()) do_write();
                } else
                    socket_.close();
            });
    }

private:
    boost::asio::io_service &io_service_;
    tcp::socket socket_;
    std::string read_msg_;
    std::deque<std::string> write_msgs_;
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
            client.write(message);

        client.close();
        th.join();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
