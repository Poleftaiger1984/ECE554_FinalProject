import argparse
import multiprocessing as mp
import os
import sys
import tempfile
import threading
import time
from pathlib import Path


def cpu_worker(duty_cycle: float, period: float) -> None:
    """
    Burn CPU in a controlled duty cycle.
    duty_cycle: 0.0 to 1.0
    period: seconds for each cycle window
    """
    busy_time = period * duty_cycle
    idle_time = max(0.0, period - busy_time)

    while True:
        start = time.perf_counter()
        while (time.perf_counter() - start) < busy_time:
            # Tight loop to consume CPU
            _ = 12345 * 67890
        if idle_time > 0:
            time.sleep(idle_time)


def start_cpu_load(target_percent: float, processes: int | None) -> list[mp.Process]:
    cpu_count = os.cpu_count() or 1

    if processes is None:
        processes = cpu_count

    processes = max(1, min(processes, cpu_count))
    duty_cycle = max(0.0, min(target_percent / 100.0, 1.0))

    procs = []
    for _ in range(processes):
        p = mp.Process(target=cpu_worker, args=(duty_cycle, 0.1), daemon=True)
        p.start()
        procs.append(p)

    print(f"[CPU] Started {len(procs)} worker process(es) at ~{target_percent:.1f}% duty each.")
    return procs


def ram_worker(target_mb: int, chunk_mb: int, hold_seconds: int) -> None:
    """
    Allocate RAM in chunks and hold it.
    """
    blocks = []
    allocated_mb = 0

    try:
        while allocated_mb < target_mb:
            blocks.append(bytearray(chunk_mb * 1024 * 1024))
            allocated_mb += chunk_mb
            print(f"[RAM] Allocated ~{allocated_mb} MB")
            time.sleep(0.2)

        print(f"[RAM] Holding ~{allocated_mb} MB for {hold_seconds} seconds...")
        if hold_seconds <= 0:
            while True:
                time.sleep(1)
        else:
            time.sleep(hold_seconds)

    except MemoryError:
        print("[RAM] MemoryError: could not allocate more RAM.")
        while True:
            time.sleep(1)


def start_ram_load(target_mb: int, chunk_mb: int, hold_seconds: int) -> mp.Process:
    p = mp.Process(target=ram_worker, args=(target_mb, chunk_mb, hold_seconds), daemon=True)
    p.start()
    print(f"[RAM] Started RAM load process targeting ~{target_mb} MB.")
    return p


def disk_writer(stop_event: threading.Event, file_path: Path, file_size_mb: int, block_mb: int) -> None:
    """
    Repeatedly write and read a temp file to generate disk activity.
    """
    block = os.urandom(block_mb * 1024 * 1024)

    while not stop_event.is_set():
        # Write phase
        written = 0
        with open(file_path, "wb", buffering=0) as f:
            while written < file_size_mb:
                f.write(block)
                written += block_mb

        # Read phase
        with open(file_path, "rb", buffering=0) as f:
            while f.read(block_mb * 1024 * 1024):
                if stop_event.is_set():
                    break


def start_disk_load(file_size_mb: int, block_mb: int, path: str | None) -> tuple[threading.Event, threading.Thread, Path]:
    if path:
        base_dir = Path(path)
        base_dir.mkdir(parents=True, exist_ok=True)
    else:
        base_dir = Path(tempfile.gettempdir())

    file_path = base_dir / "pc_load_test_temp.bin"
    stop_event = threading.Event()

    t = threading.Thread(
        target=disk_writer,
        args=(stop_event, file_path, file_size_mb, block_mb),
        daemon=True,
    )
    t.start()

    print(f"[DISK] Started disk load using file: {file_path}")
    print(f"[DISK] File size per cycle: ~{file_size_mb} MB")
    return stop_event, t, file_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate CPU, RAM, and Disk load on a Windows PC for monitor testing."
    )

    parser.add_argument("--cpu", type=float, default=0.0,
                        help="Approximate CPU load percent per worker, 0-100.")
    parser.add_argument("--cpu-procs", type=int, default=None,
                        help="Number of CPU worker processes. Default: all logical CPUs.")

    parser.add_argument("--ram-mb", type=int, default=0,
                        help="Amount of RAM in MB to allocate and hold.")
    parser.add_argument("--ram-chunk-mb", type=int, default=100,
                        help="RAM allocation chunk size in MB. Default: 100.")
    parser.add_argument("--ram-hold-seconds", type=int, default=0,
                        help="How long to hold RAM. 0 means until Ctrl+C.")

    parser.add_argument("--disk-mb", type=int, default=0,
                        help="Temp file size in MB for repeated disk write/read cycles.")
    parser.add_argument("--disk-block-mb", type=int, default=4,
                        help="Disk IO block size in MB. Default: 4.")
    parser.add_argument("--disk-path", type=str, default=None,
                        help="Directory to place temp disk test file. Default: system temp.")

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    cpu_procs: list[mp.Process] = []
    ram_proc: mp.Process | None = None
    disk_stop_event: threading.Event | None = None
    disk_thread: threading.Thread | None = None
    disk_file_path: Path | None = None

    try:
        if args.cpu > 0:
            cpu_procs = start_cpu_load(args.cpu, args.cpu_procs)

        if args.ram_mb > 0:
            ram_proc = start_ram_load(args.ram_mb, args.ram_chunk_mb, args.ram_hold_seconds)

        if args.disk_mb > 0:
            disk_stop_event, disk_thread, disk_file_path = start_disk_load(
                args.disk_mb, args.disk_block_mb, args.disk_path
            )

        if not cpu_procs and ram_proc is None and disk_thread is None:
            print("No load options selected. Example:")
            print("  python pc_load_test.py --cpu 80 --ram-mb 4000 --disk-mb 1024")
            return 1

        print("\nLoad generators are running. Press Ctrl+C to stop.\n")
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nStopping load generators...")

    finally:
        for p in cpu_procs:
            if p.is_alive():
                p.terminate()
                p.join(timeout=2)

        if ram_proc is not None and ram_proc.is_alive():
            ram_proc.terminate()
            ram_proc.join(timeout=2)

        if disk_stop_event is not None:
            disk_stop_event.set()

        if disk_thread is not None and disk_thread.is_alive():
            disk_thread.join(timeout=2)

        if disk_file_path is not None and disk_file_path.exists():
            try:
                disk_file_path.unlink()
                print(f"[DISK] Removed temp file: {disk_file_path}")
            except Exception as exc:
                print(f"[DISK] Could not remove temp file: {exc}")

        print("Done.")

    return 0


if __name__ == "__main__":
    mp.freeze_support()
    sys.exit(main())