#!/usr/bin/env python3

import argparse
import sys

import serial


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Send commands to a DLO-138 scanner over serial"
    )
    parser.add_argument("port", help="Serial port, for example COM3 or /dev/ttyUSB0")
    parser.add_argument("command", nargs="+", help="Command and optional parameters")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=10.0)
    parser.add_argument("--csv", metavar="FILE", help="Save CSV output to a file")
    args = parser.parse_args()

    command = " ".join(args.command)
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
            (i for i, line in enumerate(lines) if line.strip() == "index,mean_adc,rise_adc_per_us_x100"),
            None,
        )
        if start is None:
            parser.error("Response does not contain a waveform CSV")
        csv_lines = lines[start:]
        csv_lines = csv_lines[:next(
            (i for i, line in enumerate(csv_lines) if line.strip() == "OK"),
            len(csv_lines),
        )]
        with open(args.csv, "w", encoding="ascii", newline="\n") as csv_file:
            csv_file.write("\n".join(csv_lines) + "\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
