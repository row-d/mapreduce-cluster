#include "tcp.hpp"

namespace MapReducePICalculator
{
    class TcpServer
    {
    public:
        TcpServer(asio::io_context &io_ctx)
            : _io_ctx(io_ctx),
              _acceptor(io_ctx, tcp::endpoint(tcp::v4(), 13))
        {
            start_accept();
        }

    private:
        void start_accept()
        {
            TcpConnection::pointer conn =
                TcpConnection::create(_io_ctx);

            _acceptor.async_accept(conn->socket(),
                                   std::bind(&TcpServer::handle_accept, this, conn,
                                             asio::placeholders::error));
        }

        void handle_accept(TcpConnection::pointer conn,
                           const std::error_code &error)
        {
            if (!error)
            {
                conn->start();
            }

            start_accept();
        }

        asio::io_context &_io_ctx;
        tcp::acceptor _acceptor;
    };

} // namespace MapReducePICalculator
