#include "MFRC522.h"

static void MFRC522_WriteReg(uint8_t addr, uint8_t val)
{
    uint8_t frame[2];
    frame[0] = (addr << 1) & 0x7E;
    frame[1] = val;

    HAL_GPIO_WritePin(MFRC522_CS_GPIO_Port, MFRC522_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&MFRC522_SPI, frame, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MFRC522_CS_GPIO_Port, MFRC522_CS_Pin, GPIO_PIN_SET);
}

static uint8_t MFRC522_ReadReg(uint8_t addr)
{
    uint8_t tx[2];
    uint8_t rx[2];

    tx[0] = ((addr << 1) & 0x7E) | 0x80;
    tx[1] = 0x00;

    HAL_GPIO_WritePin(MFRC522_CS_GPIO_Port, MFRC522_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&MFRC522_SPI, tx, rx, 2, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MFRC522_CS_GPIO_Port, MFRC522_CS_Pin, GPIO_PIN_SET);

    return rx[1];
}

static void MFRC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadReg(reg);
    MFRC522_WriteReg(reg, tmp | mask);
}

static void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = MFRC522_ReadReg(reg);
    MFRC522_WriteReg(reg, tmp & (~mask));
}

void MFRC522_Reset(void)
{
    MFRC522_WriteReg(CommandReg, PCD_SOFTRESET);
    HAL_Delay(50);
}

void MFRC522_Init(void)
{
    HAL_GPIO_WritePin(MFRC522_RST_GPIO_Port, MFRC522_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);

    MFRC522_Reset();

    // Cấu hình Timer nội bộ cho MFRC522
    MFRC522_WriteReg(TModeReg, 0x8D);
    MFRC522_WriteReg(TPrescalerReg, 0x3E);
    MFRC522_WriteReg(TReloadRegL, 30);
    MFRC522_WriteReg(TReloadRegH, 0);

    MFRC522_WriteReg(TxASKReg, 0x40);
    MFRC522_WriteReg(ModeReg, 0x3D);

    // Bật anten
    uint8_t temp = MFRC522_ReadReg(TxControlReg);
    if (!(temp & 0x03))
    {
        MFRC522_SetBitMask(TxControlReg, 0x03);
    }
}

static uint8_t MFRC522_ToCard(uint8_t command,
                              uint8_t *sendData,
                              uint8_t sendLen,
                              uint8_t *backData,
                              uint16_t *backBits)
{
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint16_t i;

    switch (command)
    {
        case PCD_MFAUTHENT:
            irqEn   = 0x12;
            waitIRq = 0x10;
            break;
        case PCD_TRANSCEIVE:
            irqEn   = 0x77;
            waitIRq = 0x30;
            break;
        default:
            break;
    }

    MFRC522_WriteReg(CommIEnReg, irqEn | 0x80);
    MFRC522_ClearBitMask(CommIrqReg, 0x80);
    MFRC522_SetBitMask(FIFOLevelReg, 0x80);

    MFRC522_WriteReg(CommandReg, PCD_IDLE);

    // Ghi data vào FIFO
    for (i = 0; i < sendLen; i++)
    {
        MFRC522_WriteReg(FIFODataReg, sendData[i]);
    }

    MFRC522_WriteReg(CommandReg, command);
    if (command == PCD_TRANSCEIVE)
    {
        MFRC522_SetBitMask(BitFramingReg, 0x80);
    }

    // Chờ hoàn thành
    i = 2000;
    do
    {
        n = MFRC522_ReadReg(CommIrqReg);
        i--;
    }
    while (i && !(n & 0x01) && !(n & waitIRq));

    MFRC522_ClearBitMask(BitFramingReg, 0x80);

    if (i != 0)
    {
        if (!(MFRC522_ReadReg(ErrorReg) & 0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01) status = MI_NOTAGERR;

            if (command == PCD_TRANSCEIVE)
            {
                n = MFRC522_ReadReg(FIFOLevelReg);
                lastBits = MFRC522_ReadReg(ControlReg) & 0x07;
                if (lastBits)
                    *backBits = (n - 1) * 8 + lastBits;
                else
                    *backBits = n * 8;

                if (n == 0) n = 1;
                if (n > 16) n = 16;

                for (i = 0; i < n; i++)
                {
                    backData[i] = MFRC522_ReadReg(FIFODataReg);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    return status;
}

uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
{
    uint8_t status;
    uint16_t backBits;

    MFRC522_WriteReg(BitFramingReg, 0x07);

    TagType[0] = reqMode;
    status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((status != MI_OK) || (backBits != 0x10))
        status = MI_ERR;

    return status;
}

uint8_t MFRC522_Anticoll(uint8_t *serNum)
{
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;

    MFRC522_WriteReg(BitFramingReg, 0x00); // clear

    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;

    status = MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
        {
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[4]) status = MI_ERR;
    }
    return status;
}

void MFRC522_Halt(void)
{
    uint8_t buff[4];
    uint16_t unLen;

    buff[0] = PICC_HALT;
    buff[1] = 0;
    // CRC bỏ qua ở đây cho đơn giản
    MFRC522_ToCard(PCD_TRANSCEIVE, buff, 2, buff, &unLen);
}
uint8_t MFRC522_ReadVersion(void)
{
    return MFRC522_ReadReg(0x37); // VersionReg = 0x37
}
