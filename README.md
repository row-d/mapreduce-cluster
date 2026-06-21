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

### Build local (desarrollo)

```bash
cmake --preset=default
cmake --build build
```

Requisitos:
- C++20
- asio (via vcpkg)
- CMake >= 3.10

### Build multi-plataforma con Docker

Genera binarios para Linux x64, Linux ARM64 y Windows x64 usando Docker buildx:

```bash
# Buildear todas las plataformas
./build-all.sh

# Buildear una plataforma específica
./build-all.sh linux-x64
./build-all.sh linux-arm64
./build-all.sh windows-x64

# Listar plataformas disponibles
./build-all.sh --list
```

Los binarios se generan en `dist/<plataforma>/`.

Requisitos:
- Docker Desktop (incluye buildx)
- O docker-ce + docker-buildx-plugin

**Nota:** El build de macOS requiere un runner macOS real (GitHub Actions o Mac física) por el licenciamiento del SDK de Apple.

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

## Cambios realizados

### Implementacion inicial
- Servidor TCP async con asio que acepta workers, asigna tareas y reduce resultados.
- Cliente TCP que se conecta, recibe una tarea, ejecuta Monte Carlo localmente y envia el resultado.
- Protocolo binario con magic number, opcode y longitud de payload.
- Calculo de pi por Monte Carlo: genera puntos aleatorios en [0,1)^2 y cuenta caen dentro del circulo unitario.

### Bugs corregidos
- **`get_uint64`/`set_uint64`**: shifts imposibles (`src[0] << 64` sobre `uint64_t` = UB). Corregida la tabla de shifts a 56,48,40,32,24,16,8,0.
- **`get_type`/`set_type`**: codigo muerto con variables inexistentes y underflow de `size_t`. Eliminado.
- **`ERROR` macro collision**: `wingdi.h` define `#define ERROR 0`. Renombrado a `OPCODE_ERROR`.
- **`frame.hpp` sin include guard**: causaba redefinicion al incluirse desde multiples archivos. Anadido `#pragma once`.
- **`recv_reducer_result`**: recibia datos del worker pero los descartaba. Ahora `submit_result` acumula en `JobState`.
- **Puerto 13**: requiere root en Unix. Cambiado a 1337.
- **Faltaba `mswsock.lib`**: `AcceptEx` requiere enlazar `mswsock` en Windows.

### Separacion en dos binarios
- `mapreduce-server`: solo arranca el servidor y espera workers.
- `mapreduce-client`: solo ejecuta un worker contra un host dado.
- `PROTOCOL_PORT` movido a `frame.hpp` para que ambos binarios lo compartan.

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
