# Scoreboard Console (ESP-NOW) — Controller

Pseudocode Plan (detailed)
- Initialize serial at 115200 for debug output.
- Initialize the onboard NeoPixel and set brightness.
- Configure LED_PIN as output (status LED).
- Configure all push buttons (btn1..btn8 and BOOT) as INPUT_PULLUP.
- Set WiFi mode to WIFI_STA and print MAC address.
- Initialize ESP-NOW and register callbacks:
  - OnDataSent: update status LED / NeoPixel to green on success; record last send time.
  - OnDataRecv: copy incoming bytes into display_received struct and log ID + instruction.
- Create a peer entry containing the receiver's MAC (broadcastAddress1) and add it to ESP-NOW peers.
- Main loop:
  - Poll each button (digitalRead). For each pressed button:
    - Prepare display_packet with device ID and corresponding instruction constant.
    - Call esp_now_send to send the packet.
    - Set NeoPixel to red (or yellow for Boot button) as immediate feedback.
    - Debounce: record millis, wait until button released, delay(debounceDelay).
  - If more than 400ms passed since last OnDataSent success, set status LED off and show idle NeoPixel color.
- send_wifinow_inst(instr, ID):
  - Populate struct_packet and call esp_now_send to broadcastAddress1.
  - Log error if send returns non-ESP_OK.

Project overview
This repository contains firmware for a small ESP32-based scoreboard controller that uses push buttons to send commands to a scoreboard via ESP-NOW. The controller also provides visual feedback using an onboard NeoPixel and a status LED.

Key features
- Uses ESP-NOW for low-latency, connectionless transmissions.
- Visual feedback via onboard NeoPixel and status LED.
- Simple struct-based packet format for commands (uint16_t instruction + uint8_t id).
- Supports multiple buttons mapped to different scoreboard instructions.

Hardware
- ESP32 (S3 tested; pins referenced are for an ESP32-S3 board)
- Onboard NeoPixel (RGB_PIN = 48 in project)
- Status LED connected to LED_PIN (GPIO 10)
- Momentary push buttons wired to:
  - btn1 = GPIO 14
  - btn2 = GPIO 13
  - btn3 = GPIO 12
  - btn4 = GPIO 11
  - btn5 = GPIO 3
  - btn6 = GPIO 8
  - btn7 = GPIO 18
  - btn8 = GPIO 17
  - Boot button = GPIO_NUM_0
- Ensure buttons pull inputs to GND when pressed; code uses INPUT_PULLUP.

Software / Libraries
- Arduino core for ESP32 (or ESP32-S3) — ensure board package installed in Arduino IDE or PlatformIO.
- Adafruit NeoPixel library
- ESP-NOW (provided by esp32 Arduino core via <esp_now.h> and <WiFi.h>)

Pin mapping (from source)
- LED_PIN: 10
- RGB_PIN (NeoPixel): 48
- btn1..btn8: 14, 13, 12, 11, 3, 8, 18, 17
- Boot button: GPIO_NUM_0

Network / Addressing
- Receiver MAC address is set in source as broadcastAddress1:
  - 80:64:6F:EE:70:B0
- Change this MAC to match your scoreboard receiver before deploying.
- device_addr variable in code is the sender ID sent with every packet (default = 1).

Packet format
- C struct used:
  - uint16_t insrtuction  (note the typo preserved from code)
  - uint8_t id
- Each button maps to an instruction constant:
  - INSTR_1 = 0xA151
  - INSTR_2 = 0xA152
  - INSTR_3 = 0xA153
  - INSTR_4 = 0xA154
  - INSTR_5 = 0xA155
  - INSTR_6 = 0xA156
  - INSTR_7 = 0xA157
  - INSTR_8 = 0xA158

Build & Flash (Arduino IDE)
1. Install "ESP32" board support via Boards Manager.
2. Install "Adafruit NeoPixel" library via Library Manager.
3. Open `scoreboard_console_buttons_ESP_NOW.ino`.
4. Edit `broadcastAddress1` to contain your receiver's MAC.
5. Select the correct ESP32 board and flash settings.
6. Upload to the device.

Build & Flash (PlatformIO)
1. Create a PlatformIO project for an ESP32-S3 board.
2. Copy the .ino file into src/.
3. Add `Adafruit_NeoPixel` to lib_deps if using platformio.ini dependency management.
4. Upload with `platformio run -t upload`.

Usage
- Power the controller and ensure the receiver (scoreboard) is on and ready to accept ESP-NOW packets.
- Press any mapped button to send the corresponding command.
- The NeoPixel will turn red on button action; on successful send the OnDataSent handler will show green briefly and toggle the status LED.
- Serial output (115200) provides debug messages: packet lengths, device ID, instruction values.

Troubleshooting
- No sends / "Error sending the data":
  - Verify the broadcastAddress1 MAC matches the receiver.
  - Ensure both devices are on the same WiFi channel or use channel 0 (peerInfo.channel = 0) — explicit channel sync can be required.
- NeoPixel not lighting:
  - Verify RGB_PIN is correct for your board.
  - Confirm NeoPixel power and ground wiring.
  - Some boards require a logic-level-safe data pin; use a resistor in series (~330Ù) if needed.
- ESP-NOW init fails:
  - Check WiFi.mode(WIFI_STA) and ensure board WiFi is functioning.
  - Try resetting the board and re-flashing.

Contributing
- Bug reports and pull requests are welcome.
- Keep changes minimal and document any protocol changes (instruction codes, packet structure).

License
MIT License — see LICENSE file (or add one) when publishing.

Acknowledgements
- Adafruit NeoPixel library authors
- Espressif for ESP-NOW support in Arduino core

Minimal example to edit before publishing
- Update `broadcastAddress1` with your scoreboard MAC.
- Optionally set `device_addr` to a unique ID for this controller.
