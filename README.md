# Auto Pet Feeder

A C++ project for an automated pet feeding system, designed to ensure your pets are fed on schedule even when you're not at home. This repository provides the core logic and control code for building your own auto pet feeder, suitable for hobbyists, DIYers, and anyone interested in IoT-based pet care.

## Features

- **Scheduled Feeding:** Set and manage multiple feeding times per day.
- **Portion Control:** Dispense precise amounts of food for each meal.
- **Manual Feed Option:** Instantly feed your pet with a button press or software command.
- **LCD/LED Display Support:** Show current time, next feeding, and other status messages (if hardware available).
- **Error Handling:** Detect and report jams, low food levels, or motor errors.
- **Modular Design:** Easily adaptable for different hardware configurations (motors, sensors, displays).

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Contributing](#contributing)
- [License](#license)

## Hardware Requirements

- Microcontroller (e.g., Arduino UNO, ESP32, or similar with C++ support)
- Stepper or servo motor for food dispensing mechanism
- Real-Time Clock (RTC) module for precise scheduling
- LCD or LED display (optional)
- Buttons for manual feed/reset (optional)
- Food level sensor (optional)
- Buzzer or indicator LEDs (optional)
- Power supply suitable for your setup

> **Note:** The code is modular and can be adapted for different hardware. Check the `src/` directory and comments for pin mappings and hardware-specific notes.

## Software Requirements

- C++17 or higher
- Arduino IDE (if using Arduino/ESP32) or PlatformIO
- Required libraries (see below)

### Arduino Libraries

- `RTClib` for RTC support
- `LiquidCrystal` or `Adafruit_LiquidCrystal` for LCDs (optional)
- `AccelStepper` or similar for motor control
- Any additional libraries for sensors or displays you use

## Installation

1. **Clone this repository:**
   ```bash
   git clone https://github.com/mridul-santhosh/auto-pet-feeder.git
   cd auto-pet-feeder
   ```

2. **Open the project in your IDE:**
   - For Arduino IDE: Open the `.ino` file from the `src/` folder.
   - For PlatformIO: Open the project folder.

3. **Install dependencies:**
   - Use the Arduino Library Manager or PlatformIO to install required libraries.

4. **Configure the hardware pins and settings:**
   - Edit the configuration section in the main code file to match your hardware setup.

5. **Upload to your microcontroller:**
   - Connect your board and upload the code.

## Usage

1. **Power on the device.**
2. The device will initialize and display the current time or status.
3. Scheduled feedings will occur automatically at the set times.
4. Use the manual feed button or send a software command to trigger immediate feeding.
5. Monitor the display for status updates and errors.

## Configuration

- **Feeding Times:** Edit the predefined feeding times in the code, or implement a menu interface with buttons/display.
- **Portion Size:** Adjust the steps/seconds for the motor to control the amount of food dispensed.
- **Error Handling:** Connect sensors for food level and motor jams for enhanced safety.

### Example Configuration Section (in code)
```cpp
// Feeding schedule (24-hour format, [hour, minute])
const int feedingTimes[][2] = {
  {8, 0},   // 08:00 AM
  {18, 0}   // 06:00 PM
};
const int numFeedings = sizeof(feedingTimes) / sizeof(feedingTimes[0]);

// Portion size configuration
#define MOTOR_STEPS_PER_FEED 200  // Adjust based on your mechanism
```

## Contributing

Contributions, bug reports, and feature requests are welcome!

1. Fork the repository.
2. Create a new branch: `git checkout -b feature/my-feature`.
3. Commit your changes: `git commit -am 'Add new feature'`.
4. Push to the branch: `git push origin feature/my-feature`.
5. Open a pull request.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Author:** [mridul-santhosh](https://github.com/mridul-santhosh)

> *Automate your pet's care, and never miss a meal!*
