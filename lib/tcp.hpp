// tcp.hpp
#pragma once
#include <memory>
#include <vector>
#include <iostream>
#include <asio.hpp>
// #include <arpa/inet.h> // Para ntohs/htons (si estás en Linux/macOS)
#include "frame.hpp"

namespace MapReduce
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
            // Lectura de 6 bytes del Frame (Cabecera)
            asio::async_read(socket_,
                             asio::buffer(&header_in_, sizeof(Frame)),
                             std::bind(&TcpConnection::handle_read_header, shared_from_this(),
                                       asio::placeholders::error,
                                       asio::placeholders::bytes_transferred));
        }

        void handle_read_header(const std::error_code &error, size_t bytes_transferred)
        {
            if (!error)
            {
                // 1. Validar el Magic Number (ej: 0x4142)
                if (ntohs(header_in_.magic) != 0x4142)
                {
                    std::cerr << "[Error] Magic number inválido. Desconectando." << std::endl;
                    return; // Detiene el ciclo, destruyendo la conexión de forma segura
                }

                // 2. Obtener tamaño del payload usando tu función helper
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
                    // Es un mensaje sin cuerpo (Ej: Heartbeat), lo procesamos directo
                    process_packet();
                    read_header(); // Volvemos a escuchar la siguiente cabecera
                }
            }
            else
            {
                std::cerr << "[Info] Conexión terminada por el cliente: " << error.message() << std::endl;
            }
        }

        void read_payload()
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
            case FrameOpcode::REGISTER:
                // guardar conexión en una lista de Workers
                break;

            case FrameOpcode::MAP_RESULT:
                if (payload_in_.size() == sizeof(uint64_t))
                {
                    uint64_t puntos_dentro;
                    std::memcpy(&puntos_dentro, payload_in_.data(), sizeof(uint64_t));

                    // Ojo: Si el Worker mandó el uint64_t en Big Endian,
                    // debes hacerle un reverse/swap de bytes aquí según tu arquitectura.
                }
                break;

            default:
                // std::cout << "-> Opcode desconocido." << std::endl;
                break;
            }
        }

        tcp::socket socket_;
        Frame header_in_;
        std::vector<uint8_t> payload_in_;
    };
} // namespace MapReduce