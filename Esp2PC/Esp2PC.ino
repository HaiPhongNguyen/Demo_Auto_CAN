#define RXD2 16
#define TXD2 17

uint8_t buffer[16];

void setup() {
  Serial.begin(115200);     // PC
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // STM32
}

void loop() {
  if (Serial2.available() >= 3) {
    Serial2.readBytes(buffer, 3);

    uint16_t can_id = (buffer[0] << 8) | buffer[1];
    uint8_t dlc = buffer[2];

    while (Serial2.available() < dlc);
    Serial2.readBytes(&buffer[3], dlc);

    // ===== 2. DATA CHO PYTHON =====
    Serial.print(can_id);
    for (int i = 0; i < dlc; i++) {
      Serial.print(",");
      Serial.print(buffer[3 + i]);
    }
    Serial.println(); // rất quan trọng
  }
}