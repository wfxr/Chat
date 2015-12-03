#include "chat_room.h"
#include "chat_session.h"
#include <boost/asio.hpp>
#include <list>

using boost::asio::ip::tcp;

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
