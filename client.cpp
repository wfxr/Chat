#include "session.h"
#include <boost/asio.hpp>
#include <future>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

using boost::asio::ip::tcp;

class Client {
public:
    Client(const std::string &host, const std::string &port)
        : host_(host), port_(port) {
        CreateChatSession();
    }

    void Start() { ioService_.run(); }

    void Close() { session_.reset(); }

    void Commit(const std::string &msg) { session_->Push(msg); }

private:
    void CreateChatSession() {
        tcp::socket socket(ioService_);
        tcp::resolver resolver(ioService_);
        auto endpoint_iterator = resolver.resolve({host_, port_});
        socket.connect(*endpoint_iterator);
        session_ = std::make_shared<Session>(
            std::move(socket),
            [](const std::string &msg) { std::cout << msg << std::endl; });
    }

    std::string host_;
    std::string port_;
    boost::asio::io_service ioService_;
    std::shared_ptr<Session> session_;
};

int main(int argc, char *argv[]) {
    try {
        std::string host, port;
        if (argc == 2) {
            host = "localhost";
            port = argv[1];
        } else if (argc == 3) {
            host = argv[0];
            port = argv[1];
        } else {
            std::cerr << "Usage: " << argv[0] << " [<host>] <port>"
                      << std::endl;
            return 1;
        }

        Client client(host, port);
        std::thread thread([&client] { client.Start(); });
        std::string message;
        while (std::getline(std::cin, message))
            client.Commit(message);

        client.Close();
        thread.join();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
