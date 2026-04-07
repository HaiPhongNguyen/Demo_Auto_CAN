/*
 * MFRC522.h
 *
 *  Created on: Nov 24, 2025
 *      Author: Phong
 */

#ifndef INC_MFRC522_H_
#define INC_MFRC522_H_

#include "main.h"

// SPI handle
extern SPI_HandleTypeDef hspi1;
#define MFRC522_SPI        hspi1

// Chân CS (SS) và RST – sửa cho đúng với CubeMX
#define MFRC522_CS_GPIO_Port   GPIOA
#define MFRC522_CS_Pin         GPIO_PIN_4

#define MFRC522_RST_GPIO_Port  GPIOB
#define MFRC522_RST_Pin        GPIO_PIN_10

// Các định nghĩa lệnh / thanh ghi cơ bản
// Command set
#define PCD_IDLE              0x00
#define PCD_MEM               0x01
#define PCD_GENERATE_RANDOM_ID 0x02
#define PCD_CALCCRC           0x03
#define PCD_TRANSMIT          0x04
#define PCD_NO_CMD_CHANGE     0x07
#define PCD_RECEIVE           0x08
#define PCD_TRANSCEIVE        0x0C
#define PCD_MFAUTHENT         0x0E
#define PCD_SOFTRESET         0x0F

// PICC command
#define PICC_REQIDL           0x26
#define PICC_ANTICOLL         0x93
#define PICC_SElECTTAG        0x93
#define PICC_HALT             0x50

// Register addresses
#define CommandReg            0x01
#define CommIEnReg            0x02
#define DivIEnReg             0x03
#define CommIrqReg            0x04
#define ErrorReg              0x06
#define Status1Reg            0x07
#define Status2Reg            0x08
#define FIFODataReg           0x09
#define FIFOLevelReg          0x0A
#define ControlReg            0x0C
#define BitFramingReg         0x0D
#define ModeReg               0x11
#define TxControlReg          0x14
#define TxASKReg              0x15
#define TModeReg              0x2A
#define TPrescalerReg         0x2B
#define TReloadRegH           0x2C
#define TReloadRegL           0x2D

// Mã trả về
#define MI_OK                 0
#define MI_NOTAGERR           1
#define MI_ERR                2

void MFRC522_Init(void);
void MFRC522_Reset(void);
uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t MFRC522_Anticoll(uint8_t *serNum);
void MFRC522_Halt(void);
uint8_t MFRC522_ReadVersion(void);


#endif /* INC_MFRC522_H_ */
