#define RXD2 16
#define TXD2 17

uint8_t buffer[32];

// buffer nhận lệnh từ PC
String cmdBuffer = "";

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
}

// ================== GỬI CAN FRAME ==================
void sendCAN(uint16_t id, uint8_t *data, uint8_t len)
{
  uint8_t frame[16];

  frame[0] = (id >> 8) & 0xFF;
  frame[1] = id & 0xFF;
  frame[2] = len;

  for (int i = 0; i < len; i++) {
    frame[3 + i] = data[i];
  }

  Serial2.write(frame, 3 + len);
}

// ================== XỬ LÝ CMD ==================
void handleCommand(String cmd)
{
  if (!cmd.startsWith("CMD")) return;

  int first = cmd.indexOf(',');
  int second = cmd.indexOf(',', first + 1);

  if (first == -1 || second == -1) return;

  String direction = cmd.substring(first + 1, second);
  int speed = cmd.substring(second + 1).toInt();
  speed = constrain(speed, 0, 255);

  uint8_t data[2];

  if (direction == "FWD") data[0] = 1;
  else if (direction == "REV") data[0] = 2;
  else {
    data[0] = 0;
    speed = 0;
  }

  data[1] = speed;

  sendCAN(0x105, data, 2);

  // debug
  Serial.println("CMD OK");
}

// ================== LOOP ==================
void loop()
{
  // ==================================================
  // 1. STM32 → ESP32 → PC (NON-BLOCKING)
  // ==================================================
  if (Serial2.available() >= 3)
  {
    Serial2.readBytes(buffer, 3);

    uint16_t can_id = (buffer[0] << 8) | buffer[1];
    uint8_t dlc = buffer[2];

    // chống lỗi dlc
    if (dlc > 8) return;

    unsigned long start = millis();

    // chờ data nhưng có timeout
    while (Serial2.available() < dlc) {
      if (millis() - start > 50) return; // timeout 50ms
    }

    Serial2.readBytes(&buffer[3], dlc);

    // gửi lên PC
    Serial.print(can_id);
    for (int i = 0; i < dlc; i++) {
      Serial.print(",");
      Serial.print(buffer[3 + i]);
    }
    Serial.println();
  }

  // ==================================================
  // 2. PYTHON → ESP32 → STM32 (NON-BLOCKING)
  // ==================================================

    if (Serial.available()) {
      uint8_t dataByte = Serial.read();

      Serial2.write(dataByte);
    }
}