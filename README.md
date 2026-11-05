# OSI 7-Layer Communication Simulation

A small educational simulation of seven-layer encapsulation and decapsulation using a C TCP client/server and a Python terminal visualizer. The Python app starts both C programs locally and displays progress after each pair of OSI layers. It also prints the current username and timestamp for an assignment screenshot.

## Requirements

- GCC
- GNU Make
- Python 3.8 or newer

The Python visualizer uses only the standard library.

## Build and run

```bash
make
make run
```

Send a custom message:

```bash
python3 python/visualizer.py --message "Hello from my computer"
```

The visualizer starts a one-request server on an available local port, runs the client against it, prints the client-to-server and server-to-client layer flows, and then exits.

To run the C programs separately, open two terminals:

```bash
./build/osi-server --port 5000
```

```bash
./build/osi-client --host 127.0.0.1 --port 5000 "Hello OSI"
```

The server handles one client request and exits. The client prints the server acknowledgement after checking every layer header.

## Layer model

Each simulated layer adds a 9-byte header containing its layer number, the enclosed payload length, and a checksum. The receiver checks the expected reverse layer order, lengths, and checksums before delivering the application data. The C programs transmit length-prefixed frames over a real TCP socket; TCP is the local transport used to carry this educational seven-layer frame.

This is a teaching simulation, not a replacement for Ethernet, IP, or the operating system's networking stack. The simulated headers are application-level data sent over the host's normal TCP/IP stack.

## Test

```bash
make test
```

## Assignment screenshot

Run `make run` in a terminal wide enough to show the full output. The first line includes the logged-in username and local date/time; the output also shows the full round trip and layer progress. Capture that terminal window for submission.