# ESP Bitcoin miner
Mine Bitcoin with a low-cost microcontroller by collaborating in a low-difficulty pool.

This project’s main goal is to promote learning about how blockchains and the Bitcoin mining process work.

The code was developed in C++ with a focus on integration and execution on the ESP32 T-Display S3 circuit board. This affordable platform allows you to explore the concepts of mining and decentralization without requiring advanced hardware.

## Instructions

### How to Run the Project

**Clone the repository:** Download the project code to your local machine.

**Install a development IDE:** Use [Arduino IDE](https://www.arduino.cc/en/software) or another platform of your choice, such as [Thonny](https://thonny.org/).

**Open the project:**
- Navigate to the `esp_btc_miner.ino` file and open it in the chosen IDE.
- Edit the constants in the code to configure settings like your Wi-Fi network’s SSID and password.

**Set up the development environment:**
- Install support for the ESP32 board version 2.0.14 in the IDE (available in the Boards Manager).
- Install the required libraries:
  - **TFT_eSPI** (version 2.5.0).
  - **ArduinoJson** (version 7.2.1).

**Upload the code to the ESP32:** Compile and upload the code to the ESP32 using a USB cable.

**Power up the ESP32:** After uploading the project, connect the ESP32 to a power source. If everything is set up correctly, the following screen should appear:

![ESP32 Screen](https://i.imgur.com/Bx2byLE.png)

### Performance
- **Mining speed:** The ESP32 board reaches approximately **26.9 Kh/s**.
- **Improvement potential:** With additional adjustments and studies, the HashRate can potentially increase to **80 Kh/s**.

### Notes

This project is designed for learning and experimentation. While real mining is technically possible, the limited performance of the ESP32 makes it more suitable for understanding the fundamentals of the mining process in blockchains.

## Roadmap

- [x] Add combined hashrate for all threads
- [ ] Add compatibility with other pools
- [ ] Compatibility for cases where the pool does not have a version mask
- [ ] Increase the Hash rate to 80 Kh/s
