#pragma once
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <random>
#include <condition_variable>
#include <vector>
#include <cstring>
#include <iostream>
#include <functional>

#include <asio.hpp>
#include "frame.hpp"

namespace MapReducePICalculator
{
    using tcp = asio::ip::tcp;

    struct JobState
    {
        std::atomic<uint64_t> total_points{0};
        std::atomic<uint64_t> points_inside{0};
        std::mutex mu; // solo para completed_tasks + done + cv
        uint32_t total_tasks = 0;
        uint32_t completed_tasks = 0;
        bool done = false;
    };

    class TcpServer
    {
    public:
        TcpServer(asio::io_context &io_ctx, uint32_t num_workers, uint64_t points_per_worker)
            : _io_ctx(io_ctx),
              _acceptor(io_ctx, tcp::endpoint(tcp::v4(), PROTOCOL_PORT)),
              _job{}
        {
            _job.total_tasks = num_workers;
            std::mt19937_64 rng(42);
            for (uint32_t i = 0; i < num_workers; i++)
            {
                _tasks.push_back({rng(), points_per_worker});
            }
            start_accept();
        }

        void wait_until_done()
        {
            std::unique_lock<std::mutex> lock(_job.mu);
            _cv.wait(lock, [this]
                     { return _job.done; });
        }

        double pi_estimate() const
        {
            return 4.0 * static_cast<double>(_job.points_inside) / _job.total_points;
        }

    private:
        friend class TcpConnection;

        void start_accept();

        bool assign_task(uint32_t &worker_id, uint64_t &seed, uint64_t &points)
        {
            uint32_t idx = _next_task.fetch_add(1, std::memory_order_relaxed);
            if (idx >= _tasks.size())
                return false;
            worker_id = idx + 1;
            seed = _tasks[idx].first;
            points = _tasks[idx].second;
            return true;
        }

        void submit_result(uint64_t inside, uint64_t total)
        {
            _job.points_inside.fetch_add(inside, std::memory_order_relaxed);
            _job.total_points.fetch_add(total, std::memory_order_relaxed);

            std::lock_guard<std::mutex> lock(_job.mu);
            _job.completed_tasks++;
            if (_job.completed_tasks >= _job.total_tasks)
            {
                _job.done = true;
                _cv.notify_all();
            }
        }

        asio::io_context &_io_ctx;
        tcp::acceptor _acceptor;
        JobState _job;
        std::vector<std::pair<uint64_t, uint64_t>> _tasks;
        std::atomic<uint32_t> _next_task{0};
        std::condition_variable _cv;
    };

    class TcpConnection
        : public std::enable_shared_from_this<TcpConnection>
    {
    public:
        typedef std::shared_ptr<TcpConnection> pointer;

        static pointer create(asio::io_context &io_ctx, TcpServer *server)
        {
            return pointer(new TcpConnection(io_ctx, server));
        }

        tcp::socket &socket() { return _socket; }
        void start() { read_header(); }

    private:
        TcpConnection(asio::io_context &io_ctx, TcpServer *server)
            : _socket(io_ctx), _server(server) {}

        void read_header()
        {
            asio::async_read(_socket,
                             asio::buffer(&_header, sizeof(InputFrame)),
                             [self = shared_from_this()](const std::error_code &ec, size_t)
                             {
                                 if (ec)
                                     return;
                                 self->handle_read_header();
                             });
        }

        void handle_read_header()
        {
            if (ntohs(_header.magic) != MAGIC_NUMBER)
            {
                std::cerr << "[server] Bad magic, dropping\n";
                return;
            }

            uint32_t len = get_uint24(_header.length);
            if (len > 0)
            {
                _payload.resize(len);
                asio::async_read(_socket,
                                 asio::buffer(_payload.data(), _payload.size()),
                                 [self = shared_from_this()](const std::error_code &ec, size_t)
                                 {
                                     if (!ec)
                                         self->process_packet();
                                 });
            }
            else
            {
                process_packet();
            }
        }

        void process_packet()
        {
            switch (_header.opcode)
            {
            case REGISTER:
                handle_register();
                break;
            case MAP_RESULT:
                handle_map_result();
                break;
            case HEARTBEAT:
                read_header();
                break;
            default:
                std::cerr << "[server] Unknown opcode " << (int)_header.opcode << "\n";
                break;
            }
        }

        void handle_register()
        {
            uint32_t id;
            uint64_t seed, points;
            if (!_server->assign_task(id, seed, points))
            {
                send_error("No tasks available");
                return;
            }

            uint8_t buf[16];
            set_uint64(buf, seed);
            set_uint64(buf + 8, points);

            send_frame(REGISTER_ACK, buf, 16, [self = shared_from_this()]()
                       { self->read_header(); });
        }

        void handle_map_result()
        {
            if (_payload.size() != 16)
            {
                std::cerr << "[server] Bad MAP_RESULT size\n";
                return;
            }
            uint64_t inside = get_uint64(_payload.data());
            uint64_t total = get_uint64(_payload.data() + 8);

            _server->submit_result(inside, total);

            send_frame(MAP_RESULT, nullptr, 0, [self = shared_from_this()]()
                       { self->read_header(); });
        }

        void send_error(const char *msg)
        {
            send_frame(OPCODE_ERROR,
                       reinterpret_cast<const uint8_t *>(msg),
                       strlen(msg), nullptr);
        }

        void send_frame(InputFrameOpcode op, const uint8_t *data, size_t len,
                        std::function<void()> cb)
        {
            uint8_t hdr[6];
            uint16_t magic = htons(MAGIC_NUMBER);
            std::memcpy(hdr, &magic, 2);
            hdr[2] = static_cast<uint8_t>(op);
            set_uint24(hdr + 3, static_cast<uint32_t>(len));

            _outbuf.resize(6 + len);
            std::memcpy(_outbuf.data(), hdr, 6);
            if (len > 0)
                std::memcpy(_outbuf.data() + 6, data, len);

            asio::async_write(_socket,
                              asio::buffer(_outbuf.data(), _outbuf.size()),
                              [self = shared_from_this(), cb = std::move(cb)](const std::error_code &ec, size_t)
                              {
                                  if (!ec && cb)
                                      cb();
                              });
        }

        tcp::socket _socket;
        TcpServer *_server;
        InputFrame _header;
        std::vector<uint8_t> _payload;
        std::vector<uint8_t> _outbuf;
    };

    // Definición fuera de clase, después de que TcpConnection está completo
    inline void TcpServer::start_accept()
    {
        auto conn = TcpConnection::create(_io_ctx, this);
        _acceptor.async_accept(conn->socket(),
                               [this, conn](const std::error_code &ec)
                               {
                                   if (!ec)
                                       conn->start();
                                   start_accept();
                               });
    }

} // namespace MapReducePICalculator
