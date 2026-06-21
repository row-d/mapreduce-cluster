#include "client.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{
    // Uso: mapreduce-client <host>
    std::string host = "127.0.0.1";
    if (argc >= 2) host = argv[1];

    std::cout << "=== MapReduce Worker ===\n"
              << "Connecting to " << host << ":"
              << MapReducePICalculator::PROTOCOL_PORT << "\n\n";

    asio::io_context ctx;
    MapReducePICalculator::TcpClient client(ctx, host, MapReducePICalculator::PROTOCOL_PORT);

    auto [inside, total] = client.run();

    if (total == 0)
    {
        std::cerr << "No task available (all done?)\n";
        return 1;
    }

    std::cout << "Result: " << inside << "/" << total
              << " (" << (100.0 * inside / total) << "%)\n";
    return 0;
}
