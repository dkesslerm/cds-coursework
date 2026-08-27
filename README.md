# Concurrent and Distributed Systems Coursework

Practical coursework for the *Sistemas Concurrentes y Distribuidos* (Concurrent and Distributed Systems) course at the University of Granada (Universidad de Granada). Each folder corresponds to one of the four graded practicals of the course, implemented in C++11.

## Structure

### `p1` — Threads and semaphores
POSIX threads with a custom counting-semaphore library (`scd.h` / `scd.cpp`), used to solve classic synchronization problems:
- Producer-consumer (single and multiple producers/consumers, with dynamic load balancing)
- The sleeping barber / smokers problem

`Soluciones-20251110/` contains extra worked exercises and the exercise sheet PDFs provided by the instructors.

### `p2` — Monitors
Hoare-style monitors implemented in C++11 (`HoareMonitor`, condition variables via `scd.h` / `scd.cpp`):
- Producer-consumer
- Readers-writers
- Smokers problem, solved with monitors instead of raw semaphores

`Soluciones ejercicios adicionales-20251120/` contains additional solved exercises and the exercise sheet PDF.

### `p3` — Distributed algorithms with MPI
Distributed-memory implementations using MPI (`mpicxx` / `mpirun`):
- Producer-consumer with an intermediary buffer process
- Dining philosophers (with and without a waiter/arbiter process)
- Gas station problem (one and multiple pump types)

`s3/` contains smaller MPI examples (blocking/non-blocking send-receive, probe/iprobe) used as a seminar warm-up.

### `p4` — Real-time systems
- Cyclic executive scheduling examples (`ejecutivo1.cpp`, `ejecutivo2.cpp`, `ejecutivo1-compr.cpp`)
- Exploration of C++11 `<chrono>` clocks and durations (`relojes.cpp`, `tiempos.cpp`)
- `P4_Portfolio.pdf`: the written report submitted for this practical

## Building and running

Each folder has its own `makefile`. From inside a folder:

```sh
make <target>
```

- **p1 / p2** use `g++ -std=c++11 -pthread` and compile each exercise against `scd.cpp` / `scd.h`.
- **p3** uses `mpicxx`/`mpirun` (requires an MPI implementation, e.g. Open MPI, installed and `mpirun -oversubscribe` support).
- **p4** uses plain `g++ -std=c++11`, no external dependencies.

Run `make` with no target, or open the `makefile` in each folder, to see the available targets (e.g. `pc`, `fu`, `pcm` in p1; `x`, `pc1` in p2; `pc`, `f`, `fi`, `fc` in p3; `r`, `t`, `e1`, `e2` in p4).

## Requirements

- A C++11-capable compiler (`g++`)
- `make`
- An MPI implementation (only needed for `p3`, e.g. `libopenmpi-dev` / `openmpi-bin`)
