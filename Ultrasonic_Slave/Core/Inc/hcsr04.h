/*
 * hcsr04.h
 *
 *  Created on: Apr 18, 2026
 *      Author: Phong
 */

#ifndef INC_HCSR04_H_
#define INC_HCSR04_H_

#include "main.h"

typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;

    GPIO_TypeDef *trigPort;
    uint16_t trigPin;

    uint32_t icVal1;
    uint32_t icVal2;

    uint8_t isFirstCaptured;
    float distance;

} HCSR04_IC_t;

void delay_us(uint16_t us);
void HCSR04_IC_Init(HCSR04_IC_t *sensor);
void HCSR04_IC_Trigger(HCSR04_IC_t *sensor);
void HCSR04_IC_Callback(HCSR04_IC_t *sensor);

#endif /* INC_HCSR04_H_ */
