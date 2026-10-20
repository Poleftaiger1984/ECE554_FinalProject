# External Hardware Monitor with AI Diagnostics

A PC health monitor whose display does not depend on the PC it is watching. A Python daemon on a Windows host streams CPU, RAM, disk, and temperature readings over a serial link to bare-metal ARM firmware, which draws them on its own screen. When the PC hits an anomaly, the daemon sends its recent history to the Google Gemini API and the one-line root-cause answer is shown on the monitor.

The firmware runs on an emulated Raspberry Pi 2B in QEMU. It has not been run on physical hardware.

Built for ECE 554 at the University of Michigan-Dearborn (Winter 2026).

<!-- Add a screenshot of the QEMU window showing an alarm banner: docs/screenshot.png -->
<!-- ![Monitor display](docs/screenshot.png) -->

## Why

Regular monitoring tool for consumer hardware draw their graphs using the same operating system as the system they are monitoring. When that system freezes, the monitor freezes with it and the reading from just before the failure are gone. Enterprise servers solve this with a seperate management controller and out-of-band monitors. This project is a small consumer-grade version of that idea. The display and alarm logic run on seperate hardware, so the last known readings stay on screen and a lost connection is itself shown as an alarm. The idea is that upon a crash the independent monitor could produce a report on what went wrong.


## How it works

**Host daemon (`hostMonitor.py`)**
- Reads CPU %, RAM, disk I/O (MB/s), CPU temperature (WMI), and the top three processes by CPU, once per second.
- Sends each reading as one line of text over the serial link.
- Keeps a rolling log of the last 60 readings. If CPU > 95%, temperature > 90.0 C, or RAM > 95%, it sends that log to the Gemini API and forwards the reply to the firmware. A 60-second cooldown limits API calls.

**Firmware (C and ARM assembly)**
- `start.S` sets up the exception vectors and the stacks for IRQ and supervisor mode, parks every core except core 0, clears `.bss`, and calls `kernel_main()`.
- A UART receive interrupt pushes incoming bytes into a 1,024-byte ring buffer (`ringBuffer.c`). The main loop drains the buffer, assembles lines, and parses them into a stats struct.
- It asks the GPU for a 640x480, 32-bit framebuffer through the mailbox interface, then draws bars, numbers, and alarm banners with its own pixel, rectangle, and 8x8 font routines (`render.c`, `font.h`).
- A software watchdog marks the host as disconnected if no packet arrives for 5 seconds.
- There is no C library. `memset`, `memcpy`, `str_cpy`, and `str_cmp` are implemented in `kernel.c`.
- The main loop plus one interrupt is a bare-metal design with no scheduler.

## Alarms

The display shows the highest-priority active alarm.

| Priority | Alarm | Condition |
| --- | --- | --- |
| 1 | LINK LOST | No packet for 5 seconds |
| 2 | THERMAL CRITICAL | Temperature at or above 95.0 C |
| 3 | CPU OVERLOAD | CPU above 95% |
| 4 | RAM DEPLETED | RAM at or above 95% |
| 5 | DISK THRASH | Disk I/O at or above 200 MB/s |
| 6 | THERMAL RISE | Temperature at or above 80.0 C |
| 7 | CPU LOAD HIGH | CPU above 75% |
| 8 | RAM SATURATED | RAM at or above 75% |
| 9 | DISK BUSY | Disk I/O at or above 100 MB/s | 

Thresholds are constants in `globals.h`.

## Serial protocol

Stats are sent over in frames to the hardware as one line of text ending in a newline.

- Stats frame starts `$`, comma-seperated, each field is a two-letter tag followed by the value:
   `$CU42,CT500,RT16,RU61,RM9,DI3,ANCHROME,AC12,AR840,BN...,BC...,BR...,CN...,CC...,CR...`
- AI report frame starts with `@`, followed by free text (the firmware only keeps the first 255 characters)

| Tag | Meaning |
| --- | --- |
| `CU` | CPU usage, % |
| `CT` | CPU temperature, tenths of a degree C (500 means 50.0 C) |
| `RT`, `RM`, `RU` | RAM total (GB), RAM used (GB), RAM usage (%) |
| `DI` | Disk I/O, MB/s |
| `AN`, `AC`, `AR` | Top process 1: name, CPU %, RAM MB |
| `BN`, `BC`, `BR` | Top process 2 |
| `CN`, `CC`, `CR` | Top process 3 |

## Build and run

We built this project using WSL with Ubuntu where QEMU is installed and Python 3 on Windows. QEMU emulates a RaspberryPi 2B with 1 Gigabyte of RAM.

**1. Build the firmware (in WSL)**

```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi make qemu-system-arm -y
make
```

This produces `kernel.elf`.

**2. Start the emulator (in WSL)**

```bash
qemu-system-arm -M raspi2b -m 1G -serial tcp:127.0.0.1:4321,server,nowait,nodelay -kernel kernel.elf
```

A 640x480 window opens showing the disconnected state and a link-lost banner.

**3. Start the host daemon (Windows command prompt)**

```bat
pip install psutil wmi pyserial requests
python hostMonitor.py
```

The window switches to the connected display and updates once per second.

**4. Optional: AI reports**

Open `hostMonitor.py` and set `API_KEY` to a Gemini API key. Without one, the daemon sends the message "API KEY MISSING" instead of a report.

**5. Optional: trigger alarms**

`pc_load_test.py` is our dedicated test script that generates CPU, RAM, and disk load so you can watch the alarms fire:

```bat
python pc_load_test.py --cpu 80 --ram-mb 4000 --disk-mb 1024
```

Press Ctrl+C to stop.


## Known limitations

- This project runs on QEMU only. The firmware wasn't able to be tested on real hardware.
- The watchdog is a 5 second software check and does not reset the system.
- If the thermal sensor cannot be read through WMI, the daemon reports a fixed 50.0 C placeholder.
- The ring buffer drops incoming bytes when it is full, and a line longer than 254 characters resets the line buffer, so the remainder is parsed as a new line.
- The host daemon is Windows-only (WMI).

## Possible next steps

- Test on a physical Raspberry Pi
- Use the BCM2853 hardware watchdog for a real reset.
- Read the API key from an environment table instead of hardcoding it.
- Add unit tests for the ring buffer, the packet parser and the alarm thresholds.


## Credits

Firmware, host daemon, serial protocol, and build setup by Jason Kostopoulos. Requirements documentation and system test cases by Brad Bernia. The full requirements and test plan are in the project report.
