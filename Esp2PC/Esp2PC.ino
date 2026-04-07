#define RXD2 16
#define TXD2 17

HardwareSerial STM_UART(2);

struct CAN_Frame_t
{
    uint16_t id;
    uint8_t dlc;
    uint8_t data[8];
};

CAN_Frame_t rxFrame;

void printCANFrame(CAN_Frame_t &frame)
{
    Serial.print("CAN ID: 0x");
    Serial.println(frame.id, HEX);

    Serial.print("DLC: ");
    Serial.println(frame.dlc);

    Serial.print("DATA: ");
    for (int i = 0; i < frame.dlc; i++)
    {
        if (frame.data[i] < 0x10) Serial.print("0");
        Serial.print(frame.data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    Serial.println("-------------------");
}

void setup()
{
    Serial.begin(115200);
    STM_UART.begin(115200, SERIAL_8N1, RXD2, TXD2);

    Serial.println("ESP32 CAN UART Parser Ready");
}

void loop()
{
    if (STM_UART.available() >= 3)
    {
        // đọc header
        uint8_t id_h = STM_UART.read();
        uint8_t id_l = STM_UART.read();
        uint8_t dlc  = STM_UART.read();

        // kiểm tra DLC hợp lệ
        if (dlc > 8)
        {
            Serial.println("Invalid DLC");
            return;
        }

        // chờ đủ data
        while (STM_UART.available() < dlc);

        rxFrame.id = ((uint16_t)id_h << 8) | id_l;
        rxFrame.dlc = dlc;

        for (int i = 0; i < dlc; i++)
        {
            rxFrame.data[i] = STM_UART.read();
        }

        printCANFrame(rxFrame);
    }
}