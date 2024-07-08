---

# ReliableUDP-Comm: Reliable UDP Communication Library in C

This repository contains a C library for implementing reliable communication over UDP (RUDP). It provides functionalities for sending and receiving data with reliability features over the unreliable UDP protocol.

## Features

- **Reliable Data Transfer**: Ensures reliable communication over UDP by implementing acknowledgment and retransmission mechanisms.
- **Sender and Receiver Modules**: Separate modules for sending and receiving data.
- **API for RUDP**: A simple API to integrate reliable UDP communication in other applications.

## Installation

1. Clone the repository:
    ```bash
    git clone https://github.com/WasiimSheb/ReliableUDP-Comm.git
    ```
2. Navigate to the project directory:
    ```bash
    cd ReliableUDP-Comm
    ```

## Compilation

Compile the source code using the provided `Makefile`:
```bash
make
```

## Usage

To use the library, include the `RUDP_API.h` header file in your project and link against the compiled library.


## Files and Directories

- **RUDP_API.c / RUDP_API.h**: Implementation and header files for the RUDP API.
- **RUDP_Receiver.c**: Implementation of the RUDP receiver module.
- **RUDP_Sender.c**: Implementation of the RUDP sender module.
- **Makefile**: Makefile for compiling the project.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

---
