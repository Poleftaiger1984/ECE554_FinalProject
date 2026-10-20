# External Hardware Monitor with AI Diagnostics

A PC health monitor whose display does not depend on the PC it is watching. A Python daemon on a Windows host streams CPU, RAM, disk, and temperature readings over a serial link to bare-metal ARM firmware, which draws them on its own screen. When the PC hits an anomaly, the daemon sends its recent history to the Google Gemini API and the one-line root-cause answer is shown on the monitor.

The firmware runs on an emulated Raspberry Pi 2B in QEMU. It has not been run on physical hardware.

Built for ECE 554 at the University of Michigan-Dearborn (2026).

<!-- Add a screenshot of the QEMU window showing an alarm banner: docs/screenshot.png -->
<!-- ![Monitor display](docs/screenshot.png) -->

## Why



## How it works


## Known limitations



## Possible next steps



## Credits

Firmware, host daemon, serial protocol, and build setup by Jason Kostopoulos. Requirements documentation and system test cases by Brad Bernia. The full requirements and test plan are in the project report.
