#!/usr/bin/env python3
"""Run a validator with a wall-clock limit and retain its complete output."""
import os
from pathlib import Path
import signal
import subprocess
import sys


def main():
    if len(sys.argv) < 5:
        print("Usage: run-validator.py TIMEOUT_SECONDS LOG COMMAND [ARGS...]", file=sys.stderr)
        return 2
    timeout = float(sys.argv[1])
    if timeout <= 0:
        raise ValueError("Timeout must be positive")
    log = Path(sys.argv[2])
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w") as output:
        print("Command:", repr(sys.argv[3:]), file=output, flush=True)
        try:
            process = subprocess.Popen(sys.argv[3:], stdout=output,
                                       stderr=subprocess.STDOUT, start_new_session=True)
        except OSError as error:
            print(error, file=output)
            status = 127
        else:
            try:
                status = process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                print(f"FAIL: wall-clock timeout after {timeout:g}s", file=output, flush=True)
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
                status = 124
            if status < 0:
                status = 128 - status
        print(f"Validator exit status: {status}", file=output)
    print(log.read_text(errors="replace"), end="", flush=True)
    return status


if __name__ == "__main__":
    sys.exit(main())
