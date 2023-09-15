Transport Tycoon Simulation

This project is a simulation of train circulation inspired by the classic game "Transport Tycoon." It aims to control the movement of trains on a closed circuit using UNIX processes and various Inter-Process Communication (IPC) mechanisms such as shared memory and semaphores. Additionally, it involves applying graph theory to minimize deadlock situations.
Introduction

In this simulation, each train is controlled by a UNIX process, and IPC mechanisms are used to coordinate their movements. The main challenges include preventing accidents (train collisions) and deadlocks (trains blocking each other's path). The primary goal is to design a system that avoids accidents, and extra credit is awarded for implementing mechanisms to prevent deadlocks.
Getting Started

To compile and run the simulation, follow these steps:

Clone or download this repository.

Compile the project using the provided source file, lomo.c, and the static library liblomo.a:


    gcc lomo.c liblomo.a -o lomo

Run the program with the desired options:

To generate an HTML map:

    

    ./lomo --mapa

To run the simulation with a specified delay and the number of trains:


    ./lomo retardo nTrains

    Interact with the simulation based on the instructions provided during execution. Use CTRL+C to terminate the simulation.

Requirements

UNIX-based operating system
GCC compiler

Contributing

Contributions to this project are welcome. Feel free to open issues or submit pull requests if you find any bugs or want to improve the simulation.
Statement of practice and necessary libraries: http://avellano.usal.es/~ssooii/LOCOMOTION/lomo.htm
