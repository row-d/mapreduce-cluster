#pragma once
#include "frame.hpp"
#include <functional>
#include <iostream>
#include <random>

#include <asio.hpp>

namespace MapReducePICalculator
{
    using tcp = asio::ip::tcp;
    struct MapTask
    {
        uint64_t seed;
        uint64_t points;
    };

    class TcpClient
    {
    public:
        TcpClient(asio::io_context &io_ctx, const std::string &host, asio::ip::port_type port)
            : _socket(io_ctx), _host(host), _port(port) {}

        /// Conecta, se registra, ejecuta el map asignado y desconecta.
        /// Retorna (inside, total) o (0,0) si no hubo tarea.
        std::pair<uint64_t, uint64_t> run()
        {
            connect();
            send_register();
            auto task = recv_task();
            if (task.points == 0)
                return {0, 0};

            auto [inside, total] = monte_carlo_pi(task.seed, task.points);
            send_map_result(inside, total);
            // recv ACK
            read_frame();
            return {inside, total};
        }

    private:
        void connect()
        {
            tcp::resolver resolver(_socket.get_executor());
            auto endpoints = resolver.resolve(_host, std::to_string(_port));
            asio::connect(_socket, endpoints);
        }

        void send_register()
        {
            send_frame(InputFrameOpcode::REGISTER, nullptr, 0);
        }

        MapTask recv_task()
        {
            auto [op, payload] = read_frame();
            if (op != InputFrameOpcode::REGISTER_ACK || payload.size() != 16)
            {
                std::cerr << "[client] Unexpected response opcode=" << (int)op
                          << " size=" << payload.size() << "\n";
                return {};
            }
            MapTask t;
            t.seed = get_uint64(payload.data());
            t.points = get_uint64(payload.data() + 8);
            return t;
        }

        void send_map_result(uint64_t inside, uint64_t total)
        {
            uint8_t buf[16];
            set_uint64(buf, inside);
            set_uint64(buf + 8, total);
            send_frame(InputFrameOpcode::MAP_RESULT, buf, 16);
        }

        void send_frame(InputFrameOpcode op, const uint8_t *data, size_t len)
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

            asio::write(_socket, asio::buffer(_outbuf.data(), _outbuf.size()));
        }

        std::pair<InputFrameOpcode, std::vector<uint8_t>> read_frame()
        {
            InputFrame hdr;
            asio::read(_socket, asio::buffer(&hdr, sizeof(hdr)));

            if (ntohs(hdr.magic) != MAGIC_NUMBER)
                throw std::runtime_error("[client] Bad magic from server");

            uint32_t len = get_uint24(hdr.length);
            std::vector<uint8_t> payload;
            if (len > 0)
            {
                payload.resize(len);
                asio::read(_socket, asio::buffer(payload.data(), len));
            }
            return {hdr.opcode, std::move(payload)};
        }

        /// Monte Carlo: genera `n` puntos aleatorios en [0,1)^2 y cuenta cuántos
        /// caen dentro del círculo unitario. Usa el seed dado para reproducibilidad.
        static std::pair<uint64_t, uint64_t> monte_carlo_pi(uint64_t seed, uint64_t n)
        {
            std::mt19937_64 rng(seed);
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            uint64_t inside = 0;
            for (uint64_t i = 0; i < n; i++)
            {
                double x = dist(rng);
                double y = dist(rng);
                if (x * x + y * y <= 1.0)
                    inside++;
            }
            return {inside, n};
        }

        tcp::socket _socket;
        std::string _host;
        asio::ip::port_type _port;
        std::vector<uint8_t> _outbuf;
    };

} // namespace MapReducePICalculator
