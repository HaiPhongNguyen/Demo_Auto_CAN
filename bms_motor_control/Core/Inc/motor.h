/*
 * motor.h
 *
 *  Created on: Apr 21, 2026
 *      Author: Phong
 */

#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"

// ====== CONFIG ======
#define MOTOR_PWM_MAX 200   // chỉnh theo ARR của TIM1

// ====== API ======
void Motor_Init(TIM_HandleTypeDef *htim);

void goForward(uint16_t speed);
void goBackward(uint16_t speed);
void Stop(void);
void Brake(void);



#endif /* INC_MOTOR_H_ */
