#!/usr/bin/env python3

import argparse
import csv
import struct
import sys
import time

import serial


def capture_binary(device: serial.Serial, duration: float, output_path: str) -> int:
    device.write(b"CAPTURE\n")
    response = device.readline()
    if b"OK" not in response:
        raise RuntimeError(f"Capture was not started: {response.decode(errors='replace').strip()}")

    rows = 0
    data = bytearray()
    deadline = time.monotonic() + duration
    try:
        with open(output_path, "w", encoding="ascii", newline="") as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(("mean_mV", "delta_mV"))
            while time.monotonic() < deadline:
                data.extend(device.read(256))
                while True:
                    marker = data.find(b"\xA5\x5A")
                    if marker < 0:
                        data[:] = data[-1:] if data.endswith(b"\xA5") else b""
                        break
                    if marker > 0:
                        del data[:marker]
                    if len(data) < 6:
                        break
                    mean_mv, delta_mv = struct.unpack_from("<Hh", data, 2)
                    writer.writerow((mean_mv, delta_mv))
                    rows += 1
                    del data[:6]
    finally:
        device.write(b"R\n")
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Send commands to a DLO-138 scanner over serial"
    )
    parser.add_argument("port", help="Serial port, for example COM3 or /dev/ttyUSB0")
    parser.add_argument("command", nargs="+", help="Command and optional parameters")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--csv", metavar="FILE", help="Save CSV output to a file")
    parser.add_argument("--duration", type=float, default=5.0, help="CAPTURE duration in seconds")
    args = parser.parse_args()

    command = " ".join(args.command)
    if command.upper() == "CAPTURE":
        if not args.csv:
            parser.error("CAPTURE streams data; provide --csv FILE")
        if args.duration <= 0:
            parser.error("--duration must be positive")
        with serial.Serial(args.port, args.baud, timeout=0.1) as device:
            device.reset_input_buffer()
            rows = capture_binary(device, args.duration, args.csv)
        print(f"{rows} measurements saved to {args.csv}")
        return 0

    with serial.Serial(args.port, args.baud, timeout=args.timeout) as device:
        device.reset_input_buffer()
        device.write((command + "\n").encode("ascii"))

        response = bytearray()
        while True:
            chunk = device.readline()
            if not chunk:
                break
            response.extend(chunk)
            if b"OK\r" in chunk or b"OK\n" in chunk or b"ERROR " in chunk:
                break

    text = response.decode("ascii", errors="replace")
    sys.stdout.write(text)

    if args.csv:
        lines = text.splitlines()
        start = next(
            (
                i
                for i, line in enumerate(lines)
                if line.strip() in (
                    "mean_mV,delta_mV",
                    "index,mean_adc,rise_adc_per_us_x100",
                )
            ),
            None,
        )
        if start is None:
            parser.error("Response does not contain a CSV table")
        csv_lines = lines[start:]
        csv_lines = csv_lines[:next(
            (i for i, line in enumerate(csv_lines) if line.strip() in ("OK", "# END")),
            len(csv_lines),
        )]
        with open(args.csv, "w", encoding="ascii", newline="\n") as csv_file:
            csv_file.write("\n".join(csv_lines) + "\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
