# MapReduce Monte Carlo Pi

Calcula el numero pi usando el metodo Monte Carlo con un modelo MapReduce sobre sockets TCP (C++20 + asio).

## Arquitectura

```
  [Servidor]                          [Workers]
mapreduce-server  ←─── TCP ───→  mapreduce-client
  0.0.0.0:1337                      (N instancias)
```

El servidor divide el trabajo en N tareas (cada una con un seed y cantidad de puntos), las asigna a los workers que se conectan, y reduce los resultados para obtener la estimacion de pi.

## Build

Probado en **Windows** y **Linux (WSL)** usando vcpkg.

### Requisitos

- C++20
- CMake >= 3.19
- [vcpkg](https://vcpkg.io) con la variable de entorno `VCPKG_ROOT` configurada

### Compilar

```bash
cmake --preset=vcpkg
cmake --build build
```

Los binarios quedan en `build/mapreduce-server` y `build/mapreduce-client`.

## Uso

### Servidor

```bash
./build/mapreduce-server <num_workers> <points_per_worker>
# Ejemplo: 4 workers, 1M puntos cada uno
./build/mapreduce-server 4 1000000
```

### Worker (cliente)

```bash
./build/mapreduce-client <host>
# Ejemplo: conectar al servidor en la misma maquina
./build/mapreduce-client 127.0.0.1
# Ejemplo: conectar a otra maquina en la red
./build/mapreduce-client 192.168.1.10
```

### Flujo tipico

```bash
# Terminal 1 - Arrancar servidor
./build/mapreduce-server 4 1000000

# Terminal 2..5 - Arrancar workers (una por terminal)
./build/mapreduce-client 127.0.0.1
./build/mapreduce-client 127.0.0.1
./build/mapreduce-client 127.0.0.1
./build/mapreduce-client 127.0.0.1
```

El servidor imprime el resultado cuando todos los workers completan.

## Protocolo

Trama binaria de 6 bytes de cabecera + payload variable:

```
| magic (2B) | opcode (1B) | length (3B) | payload (length bytes) |
```

| Opcode | Direccion | Payload | Descripcion |
|--------|-----------|---------|-------------|
| 0x01 | C→S | vacio | REGISTER - worker solicita tarea |
| 0x02 | S→C | seed(8B) + points(8B) | REGISTER_ACK - servidor asigna tarea |
| 0x04 | C→S | inside(8B) + total(8B) | MAP_RESULT - worker envia resultado |
| 0x05 | S→C | vacio | MAP_RESULT ACK |
| 0x07 | S→C | string | OPCODE_ERROR |

## Estructura del proyecto

```
src/
├── server_main.cpp      # Entry point del servidor
├── client_main.cpp      # Entry point del worker
└── main.cpp             # Test local (servidor + workers en un proceso)
lib/
├── frame.hpp            # Protocolo binario, utilidades de bytes
├── server.hpp           # TcpServer + TcpConnection
└── client.hpp           # TcpClient
```

### Concurrencia optimizada

- `points_inside` y `total_points` → `atomic<uint64_t>` con `fetch_add`. Lock-free, solo se acumulan.
- `_next_task` → `atomic<uint32_t>` con `fetch_add`. Cada worker obtiene indice unico sin lock.
- Mutex reducido a proteger solo `completed_tasks` + `done` + `condition_variable` (necesitan ir juntos).
- `memory_order_relaxed` en los atomics: correcto porque los contadores son independientes y el mutex en `submit_result` ya provee el happens-before para `done = true`.

## Ejemplo de salida

```
=== MapReduce Server (Monte Carlo Pi) ===
Listening on port 1337
Waiting for 4 workers...
Points/worker: 1000000

*** Pi ≈ 3.14068 ***
Error: 0.000908654
```
