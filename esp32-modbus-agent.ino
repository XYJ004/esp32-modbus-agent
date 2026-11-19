#include <Arduino.h>

// Classic ESP32; change pins to suit your board and transceiver.
constexpr int RX_PIN = 16, TX_PIN = 17, DIR_PIN = 4;
constexpr uint8_t UNIT = 1;
constexpr uint16_t REGISTER_OFFSET = 0;
constexpr uint32_t BAUD = 9600, FRAME_GAP_US = 5000, TIMEOUT_MS = 250;
HardwareSerial rs485(2);

uint16_t modbusCrc(const uint8_t* data, size_t count) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < count; ++i) {
    crc ^= data[i];
    for (int b = 0; b < 8; ++b)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

enum class Result { Ok, Timeout, BadFrame, BadCrc, Exception };

Result readHolding(uint16_t& value, uint8_t& exception) {
  uint8_t request[] = {UNIT, 0x03, uint8_t(REGISTER_OFFSET >> 8),
                       uint8_t(REGISTER_OFFSET), 0, 1, 0, 0};
  const uint16_t crc = modbusCrc(request, 6);
  request[6] = uint8_t(crc);
  request[7] = uint8_t(crc >> 8);
  while (rs485.available()) rs485.read();
  delayMicroseconds(FRAME_GAP_US);
  digitalWrite(DIR_PIN, HIGH);   // DE high, /RE high
  delayMicroseconds(10);
  rs485.write(request, sizeof request);
  rs485.flush(true);             // Wait for the final stop bit; don't purge RX
  digitalWrite(DIR_PIN, LOW);    // DE low, /RE low

  uint8_t response[7];           // Seven bytes for one register, five for exception
  size_t count = 0;
  uint32_t start = millis(), last = micros();
  while (uint32_t(millis() - start) < TIMEOUT_MS) {
    if (count && uint32_t(micros() - last) >= FRAME_GAP_US) break;
    if (!rs485.available()) {
      delayMicroseconds(50);
      continue;
    }
    if (count == sizeof response) return Result::BadFrame;
    const int byte = rs485.read();
    if (byte < 0) continue;
    response[count++] = uint8_t(byte);
    last = micros();
  }
  if (!count) return Result::Timeout;
  if (count != 5 && count != 7) return Result::BadFrame;
  const uint16_t received = uint16_t(response[count - 2]) |
                            (uint16_t(response[count - 1]) << 8);
  if (modbusCrc(response, count - 2) != received) return Result::BadCrc;
  if (response[0] != UNIT) return Result::BadFrame;
  if (count == 5 && response[1] == 0x83) {
    exception = response[2];
    return Result::Exception;
  }
  if (count != 7 || response[1] != 0x03 || response[2] != 2)
    return Result::BadFrame;
  value = (uint16_t(response[3]) << 8) | response[4];
  return Result::Ok;
}

void setup() {
  Serial.begin(115200);
  pinMode(DIR_PIN, OUTPUT);
  digitalWrite(DIR_PIN, LOW);
  rs485.begin(BAUD, SERIAL_8E1, RX_PIN, TX_PIN);
}

void loop() {
  uint16_t value = 0;
  uint8_t exception = 0;
  switch (readHolding(value, exception)) {
    case Result::Ok: Serial.printf("Register %u = %u\n", unsigned(REGISTER_OFFSET), unsigned(value)); break;
    case Result::Timeout: Serial.println("No response"); break;
    case Result::BadFrame: Serial.println("Unexpected response frame"); break;
    case Result::BadCrc: Serial.println("CRC mismatch"); break;
    case Result::Exception: Serial.printf("Modbus exception 0x%02X\n", unsigned(exception)); break;
  }
  delay(1000);
}
