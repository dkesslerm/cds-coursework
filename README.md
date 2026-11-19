# Concurrent and Distributed Systems Coursework

My practical work for the *Sistemas Concurrentes y Distribuidos* (Concurrent and Distributed Systems) course at the University of Granada. Each folder corresponds to one of the four practicals of the course, implemented in C++11.

> **Course-provided code:** the semaphore and Hoare-monitor library (`scd.h` / `scd.cpp`, by Carlos Ureña) and the `*-plantilla.cpp` templates were provided by the course. The solutions built on top of them are my own work.

## Structure

### `p1` — Threads and semaphores
C++11 threads (`std::thread`) synchronised with the course's counting semaphores:
- Producer-consumer: single producer/consumer, and multiple producers and consumers (FIFO and LIFO buffer variants)
- Smokers problem

### `p2` — Monitors
Monitors with Hoare semantics, written using the course's `HoareMonitor` class and condition variables:
- Producer-consumer (single and multiple)
- Readers-writers
- Smokers problem, solved with monitors instead of raw semaphores

### `p3` — Distributed algorithms with MPI
Distributed-memory implementations using MPI (`mpicxx` / `mpirun`):
- Producer-consumer with an intermediary buffer process
- Dining philosophers (with and without a waiter process)
- Gas station problem (one and multiple pump types)

`s3/` contains smaller MPI examples (blocking and non-blocking send/receive, `probe`/`iprobe`) used as a seminar warm-up.

### `p4` — Real-time systems
- Cyclic executive scheduling (`ejecutivo1.cpp`, `ejecutivo2.cpp`, `ejecutivo1-compr.cpp`)
- C++11 `<chrono>` clocks and durations (`relojes.cpp`, `tiempos.cpp`)
- `P4_Portfolio.pdf`: the written report for this practical

## Building and Running

Each folder has its own `makefile`. From inside a folder:

```sh
make <target>
```

- **p1 / p2** use `g++ -std=c++11 -pthread` and compile each exercise against `scd.cpp` / `scd.h`.
- **p3** uses `mpicxx`/`mpirun` (requires an MPI implementation such as Open MPI, with `mpirun -oversubscribe` support).
- **p4** uses plain `g++ -std=c++11`, with no external dependencies.

Open the `makefile` in each folder to see the available targets.

## Requirements

- A C++11-capable compiler (`g++`)
- `make`
- An MPI implementation (only for `p3`, e.g. `libopenmpi-dev` / `openmpi-bin`)
