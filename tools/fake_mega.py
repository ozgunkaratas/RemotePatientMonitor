#!/usr/bin/env python3
"""Pretend to be the Arduino Mega, so the gateway can run without hardware.

Creates a pseudo-terminal, links it to /tmp/fake-mega and writes the phase 0
heartbeat line every 100 ms, exactly like firmware/mega does.

Linux / WSL only (uses pty). Usage:
    python3 tools/fake_mega.py               # then: rpm_gateway /tmp/fake-mega
    python3 tools/fake_mega.py --lose-every 50   # skip a heartbeat every 50 to test gap warnings
"""
import argparse
import os
import pty
import time

LINK = "/tmp/fake-mega"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--period-ms", type=int, default=100, help="heartbeat period (default 100)")
    parser.add_argument("--lose-every", type=int, default=0, help="drop every Nth heartbeat (0 = never)")
    args = parser.parse_args()

    master, slave = pty.openpty()
    if os.path.lexists(LINK):
        os.remove(LINK)
    os.symlink(os.ttyname(slave), LINK)
    print(f"fake Mega on {LINK} -> {os.ttyname(slave)}  (Ctrl-C to stop)")

    seq = 0
    start = time.monotonic()
    try:
        while True:
            uptime_ms = int((time.monotonic() - start) * 1000)
            lost = args.lose_every > 0 and seq > 0 and seq % args.lose_every == 0
            if not lost:
                line = f"HB seq={seq} uptime_ms={uptime_ms} drop=0\n"
                try:
                    os.write(master, line.encode())
                except BlockingIOError:
                    pass  # nobody reading yet; the real Mega also just keeps going
            seq += 1
            time.sleep(args.period_ms / 1000)
    except KeyboardInterrupt:
        pass
    finally:
        os.remove(LINK)


if __name__ == "__main__":
    main()
