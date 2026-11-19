# ESP32 Modbus RTU reader

A minimal, read-only Arduino sketch for a classic ESP32 and a half-duplex RS485 transceiver. It polls one holding register using function 0x03 and prints the raw 16-bit value or a diagnostic. It does not write device registers or use a network service.

## Run on a bench

1. Open `esp32-modbus-agent.ino` in Arduino IDE and select your ESP32 board.
2. With power off, wire GPIO17 (TX) to DI, GPIO16 (RX) to RO, and GPIO4 to DE and active-low /RE tied together. Use a 3.3 V logic-compatible transceiver. Match differential polarity per each device's datasheet and follow its reference-ground, isolation, and termination guidance. These GPIOs are examples; some boards reserve them.
3. Connect a known low-voltage Modbus RTU slave or simulator. Match its settings in the sketch: 9600 baud, 8E1, unit ID 1, zero-based holding-register offset 0. Check your device's register map; some manuals label offset 0 as 40001.
4. Upload the sketch and open Serial Monitor at 115200. Check the reported raw value against the slave or simulator. Unplug the slave to check timeout reporting.

The frame-silence interval in this example is chosen for 9600 baud/8E1; revisit it if you change the line settings. This example is for a single-master bench bus. No physical RS485 validation is claimed.

References: [ESP32 Arduino UART](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/serial.html), [Modbus Application Protocol §6.3](https://www.modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf).
