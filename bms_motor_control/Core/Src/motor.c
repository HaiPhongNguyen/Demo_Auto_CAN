#include "motor.h"

// ====== PRIVATE ======
static TIM_HandleTypeDef *motor_htim = NULL;

// ====== INIT ======
void Motor_Init(TIM_HandleTypeDef *htim)
{
    motor_htim = htim;

    // Start PWM 2 kênh
    HAL_TIM_PWM_Start(motor_htim, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(motor_htim, TIM_CHANNEL_2);
}

// ====== CORE CONTROL ======
void goForward(uint16_t speed)
{
    if(speed > MOTOR_PWM_MAX) speed = MOTOR_PWM_MAX;

    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_1, speed); // IN1 PWM
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_2, 0);     // IN2 LOW
}

void goBackward(uint16_t speed)
{
    if(speed > MOTOR_PWM_MAX) speed = MOTOR_PWM_MAX;

    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_1, 0);     // IN1 LOW
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_2, speed); // IN2 PWM
}

void Stop(void)
{
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_2, 0);
}

void Brake(void)
{
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_1, MOTOR_PWM_MAX);
    __HAL_TIM_SET_COMPARE(motor_htim, TIM_CHANNEL_2, MOTOR_PWM_MAX);
}

