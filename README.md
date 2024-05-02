# [Ball Algorithm Parallel and Distributed Solver](https://fenix.tecnico.ulisboa.pt/disciplinas/CPD132646/2020-2021/2-semestre)

This project implements a parallel Ball Tree construction algorithm in a Distributed-Memory environment. The implementation focuses on problem decomposition, synchronization concerns, and load balancing to improve performance.

## Contributors

* [Clara Gil](https://github.com/gil101), **Grade**: 17/20
* [José Brás](https://github.com/ist182069), **Grade**: 17/20
* [Miguel Barros](https://github.com/MVBarros), **Grade**: 19/20

## Overview
The project aims to efficiently construct a Ball Tree data structure in a distributed memory setting. The algorithm splits the point set among available processes, enabling the handling of larger problem instances that do not fit on a single machine.

## Key Features
- Problem decomposition for distributing workload among processes
- Synchronization mechanisms for efficient parallel computation
- Load balancing strategies to optimize performance

## Benchmark Results
The parallel implementation shows significant speedup compared to the sequential version, with a 36% improvement for larger inputs. The speedup scales with the number of processes up to a certain point, after which communication overhead limits further gains.

## How to Run
1. Clone the repository to your local machine.
2. Compile the project using the provided Makefile.
make

3. Run the executable with the desired input parameters.
mpirun -np <num_processes> ./ball_tree_construction <input_file>

Example:
mpirun -np 4 ./ball_tree_construction input_data.txt

4. View the output results and performance metrics.

## Source Files
- `ball_tree_construction.cpp`: Main source code file for the Ball Tree construction algorithm.
- `Makefile`: Makefile for compiling the project.
- `input_data.txt`: Sample input data file.

## Dependencies
- OpenMPI
- C/C++ compiler

## Additional Information
For more details on the project implementation, performance analysis, and algorithm insights, refer to the full report [g29report.pdf](link_to_report).
