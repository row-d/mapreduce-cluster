#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

int main(int argc, char **argv)
{
    // Uso: MapReduceSocket <num_workers> <points_per_worker>
    // Ej:  MapReduceSocket 4 1000000
    uint32_t num_workers = 4;
    uint64_t points_per_worker = 1'000'000;

    if (argc >= 3)
    {
        num_workers = static_cast<uint32_t>(std::stoul(argv[1]));
        points_per_worker = std::stoull(argv[2]);
    }

    std::cout << "=== MapReduce Monte Carlo Pi ===\n"
              << "Workers: " << num_workers << "\n"
              << "Points/worker: " << points_per_worker << "\n"
              << "Total points: " << num_workers * points_per_worker << "\n\n";

    // 1. Arrancar servidor en un hilo separado
    asio::io_context server_ctx;
    MapReducePICalculator::TcpServer server(server_ctx, num_workers, points_per_worker);
    std::thread server_thread([&server_ctx]() { server_ctx.run(); });

    // Darle un momento al servidor para que abra el acceptor
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Lanzar N workers (clientes) en paralelo
    asio::io_context client_ctx;
    std::vector<std::thread> workers;
    std::vector<std::pair<uint64_t, uint64_t>> results(num_workers);

    for (uint32_t i = 0; i < num_workers; i++)
    {
        workers.emplace_back([&, i]()
        {
            asio::io_context ctx;
            MapReducePICalculator::TcpClient client(ctx, "127.0.0.1", MapReducePICalculator::PROTOCOL_PORT);
            results[i] = client.run();
            std::cout << "  worker " << i << ": inside=" << results[i].first
                      << " total=" << results[i].second << "\n";
        });
    }

    for (auto &w : workers) w.join();

    // 3. Esperar a que el servidor reduzca
    server.wait_until_done();

    // 4. Mostrar resultado
    double pi = server.pi_estimate();
    std::cout << "\nPi ≈ " << pi << "\n";
    std::cout << "Error: " << std::abs(pi - 3.14159265358979323846) << "\n";

    server_ctx.stop();
    server_thread.join();

    // Verificación básica: pi debe estar razonablemente cerca
    assert(pi > 3.0 && pi < 3.3);
    return 0;
}
