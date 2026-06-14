#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <asio.hpp>

using asio::ip::tcp;

class TcpConnection
    : public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef std::shared_ptr<TcpConnection> pointer;

    static pointer create(asio::io_context &io_context)
    {
        return pointer(new TcpConnection(io_context));
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        message_ = "";

        asio::async_write(socket_, asio::buffer(message_),
                          std::bind(&TcpConnection::handle_write, shared_from_this(),
                                    asio::placeholders::error,
                                    asio::placeholders::bytes_transferred));
    }

private:
    TcpConnection(asio::io_context &io_context)
        : socket_(io_context)
    {
    }

    void handle_write(const std::error_code & /*error*/,
                      size_t /*bytes_transferred*/)
    {
    }

    tcp::socket socket_;
    std::string message_;
};

class TcpServer
{
public:
    TcpServer(asio::io_context &io_context)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), 13))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        TcpConnection::pointer new_connection =
            TcpConnection::create(io_context_);

        acceptor_.async_accept(new_connection->socket(),
                               std::bind(&TcpServer::handle_accept, this, new_connection,
                                         asio::placeholders::error));
    }

    void handle_accept(TcpConnection::pointer new_connection,
                       const std::error_code &error)
    {
        if (!error)
        {
            new_connection->start();
        }

        start_accept();
    }

    asio::io_context &io_context_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        asio::io_context io_context;
        TcpServer server(io_context);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}