#!/usr/bin/env python3
"""Run the local C client/server pair and visualize their OSI layer flow."""

from __future__ import annotations

import argparse
from datetime import datetime
import getpass
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
LAYER_NAMES = {
    1: "Physical",
    2: "Data Link",
    3: "Network",
    4: "Transport",
    5: "Session",
    6: "Presentation",
    7: "Application",
}
COLORS = {1: "37", 2: "31", 3: "34", 4: "33", 5: "35", 6: "36", 7: "32"}
TRACE_PATTERN = re.compile(r"^TRACE (TX|RX) ([1-7]) (.+)$")


def parse_trace(output: str, direction: str) -> list[tuple[int, str]]:
    events = []
    for line in output.splitlines():
        match = TRACE_PATTERN.match(line)
        if match and match.group(1) == direction:
            events.append((int(match.group(2)), match.group(3)))
    return events


def show_phase(title: str, events: list[tuple[int, str]], descending: bool) -> None:
    expected = list(range(7, 0, -1) if descending else range(1, 8))
    events_by_layer = {layer: name for layer, name in events}
    if list(events_by_layer) and any(layer not in events_by_layer for layer in expected):
        raise RuntimeError(f"Incomplete layer trace during {title.lower()}.")

    print(f"\n{title}")
    for index in range(0, len(expected), 2):
        group = expected[index : index + 2]
        for layer in group:
            label = f"L{layer} {events_by_layer.get(layer, LAYER_NAMES[layer])}"
            if sys.stdout.isatty():
                label = f"\033[{COLORS[layer]}m{label}\033[0m"
            arrow = "↓" if descending else "↑"
            print(f"  {label} {arrow}")
        if len(group) == 1:
            print(f"  Final layer {group[0]} complete")
        else:
            print(f"  Progress: layers {group[0]}-{group[-1]} complete")


def run_simulation(message: str) -> int:
    server = subprocess.Popen(
        [str(ROOT / "build" / "osi-server"), "--port", "0"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    try:
        ready = server.stdout.readline().strip()
        if not ready.startswith("READY "):
            _, server_error = server.communicate(timeout=5)
            raise RuntimeError(server_error.strip() or "The C server did not start.")
        port = ready.split(maxsplit=1)[1]

        client = subprocess.run(
            [str(ROOT / "build" / "osi-client"), "--host", "127.0.0.1",
             "--port", port, message],
            capture_output=True,
            text=True,
            timeout=10,
            check=False,
        )
        server_output, server_error = server.communicate(timeout=10)
        if client.returncode != 0:
            raise RuntimeError(client.stderr.strip() or "The C client failed.")
        if server.returncode != 0:
            raise RuntimeError(server_error.strip() or "The C server failed.")

        client_output = client.stdout
        show_phase("Client encapsulation (sender)", parse_trace(client_output, "TX"), True)
        show_phase("Server decapsulation (receiver)", parse_trace(server_output, "RX"), False)
        show_phase("Server encapsulation (acknowledgement)", parse_trace(server_output, "TX"), True)
        show_phase("Client decapsulation (acknowledgement)", parse_trace(client_output, "RX"), False)
        reply = next((line[7:] for line in client_output.splitlines()
                      if line.startswith("REPLY: ")), "")
        print(f"\nDelivered message: {reply}")
        return 0
    finally:
        if server.poll() is None:
            server.terminate()
            server.wait(timeout=5)


def main() -> int:
    parser = argparse.ArgumentParser(description="Visualize a seven-layer OSI socket round trip.")
    parser.add_argument("--message", default="Hello from the OSI simulation",
                        help="message sent through the C client/server")
    arguments = parser.parse_args()

    if not (ROOT / "build" / "osi-client").exists() or not (ROOT / "build" / "osi-server").exists():
        parser.error("C programs are not built; run `make` first")

    now = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")
    print(f"OSI 7-Layer Socket Simulation | User: {getpass.getuser()} | {now}")
    print(f"Message: {arguments.message}")
    try:
        return run_simulation(arguments.message)
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"Simulation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())