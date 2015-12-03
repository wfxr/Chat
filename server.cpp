#include "room.h"
#include <boost/asio.hpp>
#include <list>

using boost::asio::ip::tcp;

class ChatServer {
public:
    void CreateChatRoom(const std::string &port) {
        tcp::endpoint endpoint(tcp::v4(), std::stoi(port));
        tcp::socket socket(ioService_);
        tcp::acceptor acceptor(ioService_, endpoint);
        chat_rooms_.emplace_back(std::move(socket), std::move(acceptor));
    }

    void Start() { ioService_.run(); }

private:
    boost::asio::io_service ioService_;
    std::list<ChatRoom> chat_rooms_;
};

int main(int argc, char *argv[]) {
    try {
        if (argc < 2) {
            std::cerr << "Usage: " << argv[0] << " <port> [<port> ...]"
                      << std::endl;
            return 1;
        }

        ChatServer server;
        for (int i = 1; i < argc; ++i)
            server.CreateChatRoom(argv[i]);

        server.Start();
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
