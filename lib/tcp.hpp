// tcp.hpp
#pragma once
#include <memory>
#include <vector>
#include <iostream>
#include <asio.hpp>
#include <thread>

#ifdef __unix__
#include <arpa/inet.h> // Para ntohs/htons (Linux/macOS)
#endif
#include "frame.hpp"

namespace MapReducePICalculator
{
    using tcp = asio::ip::tcp;

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
            read_header();
        }

    private:
        TcpConnection(asio::io_context &io_context)
            : socket_(io_context)
        {
        }

        void read_header()
        {
            // Lectura de 6 bytes del InputFrame (Cabecera)
            asio::async_read(socket_,
                             asio::buffer(&header_in_, sizeof(InputFrame)),
                             std::bind(&TcpConnection::handle_read_header, shared_from_this(),
                                       asio::placeholders::error,
                                       asio::placeholders::bytes_transferred));
        }

        void handle_read_header(const std::error_code &error, size_t bytes_transferred)
        {
            if (!error)
            {
                // 1. Validar el Magic Number
                if (ntohs(header_in_.magic) != MAGIC_NUMBER)
                {
                    std::cerr << "[Error] Magic number inválido. Desconectando." << std::endl;
                    return; // Detiene el ciclo, destruyendo la conexión de forma segura
                }

                // 2. Obtener tamaño del payload
                uint32_t payload_len = get_uint24(header_in_.length);

                if (payload_len > 0)
                {
                    // Ajustamos el tamaño del buffer dinámico para recibir el payload
                    payload_in_.resize(payload_len);

                    // Pasamos a leer el resto del mensaje (Cuerpo)
                    read_payload();
                }
                else
                {
                    // Es un mensaje sin cuerpo
                    process_packet();
                    read_header(); // Volvemos a escuchar la siguiente cabecera
                }
            }
            else
            {
                std::cerr << "[Info] Conexión terminada por el cliente: " << error.message() << std::endl;
            }
        }

        inline void read_payload()
        {
            // Forzamos a leer los bytes exactos que se especificaron en la cabecera
            asio::async_read(socket_,
                             asio::buffer(payload_in_.data(), payload_in_.size()),
                             std::bind(&TcpConnection::handle_read_payload, shared_from_this(),
                                       asio::placeholders::error,
                                       asio::placeholders::bytes_transferred));
        }

        void handle_read_payload(const std::error_code &error, size_t bytes_transferred)
        {
            if (!error)
            {
                // Ya tenemos la cabecera y el cuerpo completo
                process_packet();

                // Limpiamos y volvemos a escuchar la siguiente trama en el flujo TCP
                read_header();
            }
            else
            {
                // std::cerr << "[Error] Al leer payload: " << error.message() << std::endl;
            }
        }

        void process_packet()
        {
            switch (header_in_.opcode)
            {
            case InputFrameOpcode::REGISTER:
                register_worker();
                break;

            case InputFrameOpcode::MAP_RESULT:
                recv_reducer_result();
                break;

            default:
                std::cerr << "-> Opcode desconocido." << std::endl;
                break;
            }
        }

        inline void recv_reducer_result()
        {
            if (payload_in_.size() == sizeof(uint64_t))
            {
                uint64_t puntos_dentro;
                std::memcpy(&puntos_dentro, payload_in_.data(), sizeof(uint64_t));
                // ntohl
                // # TODO: Si el Worker mandó el uint64_t en Big Endian,
                // hacer un reverse/swap de bytes aquí según arquitectura.
            }
        }

        inline void register_worker()
        {
            std::lock_guard<std::mutex> lock(workers_mutex_);
            uint32_t id = next_worker_id_++;
            workers_[id] = shared_from_this();
        }

        tcp::socket socket_;
        InputFrame header_in_;
        std::vector<uint8_t> payload_in_;
        std::unordered_map<uint32_t, pointer> workers_;
        std::mutex workers_mutex_;
        uint32_t next_worker_id_ = 1;
    };
} // namespace MapReducePICalculator
