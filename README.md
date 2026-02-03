# FocusBar

A compact Pomodoro timer device with visual LED progress bar, tactile button controls, and audio feedback. Designed to help you stay focused and productive.

![FocusBar](hardware/FocusBeam.step)

## Features

- **Visual Progress Bar**: 10 WS2812 addressable RGB LEDs show timer progress
- **5 Preset Durations**: Quick-select buttons for 5, 15, 30, 45, and 60 minute focus sessions
- **Audio Feedback**: Piezo buzzer for startup jingle, completion notification, and alert sounds
- **Intuitive States**: Color-coded LED feedback for different timer states
- **USB-C Powered**: Modern, reversible connector for easy power
- **Compact Design**: Custom PCB with 3D-printable enclosure

## Timer States

| State | LED Color | Behavior |
|-------|-----------|----------|
| Idle | Cyan (30% brightness) | All LEDs solid, ready to start |
| Running | Green | Progress bar fills from 0-100% |
| Completed | Green (pulsing) | 60-second grace period to acknowledge |
| Alerting | Red (pulsing) | Audio alert every 2 seconds until dismissed |

## Usage

1. **Start Timer**: Press any of the 5 buttons to start the corresponding timer:
   - Button 1: 5 minutes
   - Button 2: 15 minutes
   - Button 3: 30 minutes
   - Button 4: 45 minutes
   - Button 5: 60 minutes

2. **Cancel Timer**: Press any button while timer is running to stop and reset

3. **Acknowledge Completion**: Press any button during the grace period (green pulsing) to reset without triggering the alert

4. **Dismiss Alert**: Press any button during the alert (red pulsing) to silence and reset

## Hardware

### Components

| Component | Part Number | Description |
|-----------|-------------|-------------|
| MCU | ESP32-H2-MINI-1 | WiFi/BLE microcontroller module |
| LEDs | IN-PI15TAT5R5G5B | WS2812-compatible addressable RGB LEDs (x10) |
| Buttons | TL1016AAF220QG | Tactile push buttons (x5) |
| Buzzer | SMT1240SHT | SMD piezo buzzer |
| Regulator | AP2205-33W5-7 | 3.3V LDO voltage regulator |
| USB | USB4105-GF-A | USB-C connector |
| ESD Protection | TPD1E10B06 | USB ESD protection diode |

### PCB

- **Design Tool**: KiCad
- **Files Location**: `hardware/`
- **Schematic**: `FocusBeam.kicad_sch`
- **PCB Layout**: `FocusBeam.kicad_pcb`
- **Gerbers**: `gerbers/` or `FocusBeam-v1.1-fab-gerbers.zip`
- **BOM**: `FocusBeam-v1.1-BOM.csv`
- **Panel Design**: `panel/` (for production runs)

### Mechanical

3D-printable enclosure files (STEP format) located in `mechanical/`:
- `FocusBeam_v1.1_front.step` - Front cover
- `FocusBeam_v1.1_back.step` - Back cover
- `FocusBeam_v1.1_buttons.step` - Button caps
- `FocusBeam_v.1.1_stand.step` - Desktop stand

## Firmware

### Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32h2/get-started/index.html) v5.x or later
- ESP32-H2 target support

### Building

```bash
cd firmware
idf.py set-target esp32h2
idf.py build
```

### Flashing

```bash
idf.py -p (PORT) flash monitor
```

Replace `(PORT)` with your serial port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux).

### Firmware Architecture

```
firmware/
├── CMakeLists.txt          # Project build configuration
├── sdkconfig.ci            # SDK configuration
└── main/
    ├── main.c              # Application entry point and main loop
    ├── button.c/h          # Button input handling (short/long press)
    ├── led.c/h             # LED control (colors, progress, pulsing)
    ├── led_color_lib.c/h   # Color utilities
    ├── piezo.c/h           # Buzzer control (tones, melodies)
    ├── timer.c/h           # Pomodoro state machine
    ├── serial_protocol.c/h # Serial communication (future use)
    └── ws2812_control.c/h  # WS2812 LED driver (RMT peripheral)
```

### Key Modules

- **Timer**: State machine managing idle, running, completed, grace period, and alerting states
- **LED**: Thread-safe LED control with smooth transitions and pulsing effects
- **Button**: Interrupt-driven button handler with debouncing and short/long press detection
- **Piezo**: PWM-based buzzer control for tones and melodies

## Pin Configuration

| Function | GPIO |
|----------|------|
| WS2812 Data | (see ws2812_control.c) |
| Button SW0 | GPIO 10 |
| Button SW1 | GPIO 5 |
| Button SW2 | GPIO 4 |
| Button SW3 | GPIO 14 |
| Button SW4 | GPIO 13 |
| Piezo | GPIO 22 |

## Project Structure

```
FocusBar/
├── README.md               # This file
├── firmware/               # ESP-IDF firmware project
│   ├── main/               # Source code
│   └── CMakeLists.txt
├── hardware/               # KiCad PCB design files
│   ├── FocusBeam.kicad_*   # Main PCB files
│   ├── gerbers/            # Manufacturing files
│   ├── panel/              # Panelized PCB for production
│   └── FocusBeam.step      # 3D model of PCB
└── mechanical/             # 3D-printable enclosure (STEP files)
```

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

## Author

StuckAtPrototype, LLC
