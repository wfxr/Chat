#include "chat_session.h"
#include <boost/asio.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

class client {
public:
    client(const std::string &host, const std::string &port)
        : host_(host), port_(port) {
        create_chat_session();
    }

    void start() { io_service_.run(); }

    void close() { chat_session_.reset(); }

    void push(const std::string &message) { chat_session_->push(message); }

private:
    void create_chat_session() {
        tcp::socket socket(io_service_);
        tcp::resolver resolver(io_service_);
        auto endpoint_iterator = resolver.resolve({host_, port_});
        socket.connect(*endpoint_iterator);
        chat_session_ = std::make_shared<chat_session>(
            std::move(socket), [](const std::string &message) {
                std::cout << message << std::endl;
            });
    }

    std::string host_;
    std::string port_;
    boost::asio::io_service io_service_;
    std::shared_ptr<chat_session> chat_session_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;
            return 1;
        }

        std::string host(argv[1]), port(argv[2]);
        client a_client(host, port);
        std::thread th([&a_client]() { a_client.start(); });
        std::string message;
        while (std::getline(std::cin, message))
            a_client.push(message);

        a_client.close();
        th.join();

    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
