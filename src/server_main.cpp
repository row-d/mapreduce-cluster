#include "server.hpp"
#include <iostream>
#include <cstdlib>
#include <numbers>

int main(int argc, char **argv)
{
    // Uso: mapreduce-server <num_workers> <points_per_worker>
    uint32_t num_workers = 4;
    uint64_t points_per_worker = 1'000'000;

    if (argc >= 3)
    {
        num_workers = static_cast<uint32_t>(std::stoul(argv[1]));
        points_per_worker = std::stoull(argv[2]);
    }

    std::cout << "=== MapReduce Server (Monte Carlo Pi) ===\n"
              << "Listening on port " << MapReducePICalculator::PROTOCOL_PORT << "\n"
              << "Waiting for " << num_workers << " workers...\n"
              << "Points/worker: " << points_per_worker << "\n\n";

    asio::io_context ctx;
    MapReducePICalculator::TcpServer server(ctx, num_workers, points_per_worker);

    server.wait_until_done();

    double pi = server.pi_estimate();
    std::cout << "\n*** Pi ≈ " << pi << " ***\n";
    std::cout << "Error: " << std::abs(pi - std::numbers::pi) << "\n";

    return 0;
}
