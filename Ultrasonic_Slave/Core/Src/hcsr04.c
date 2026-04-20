#include "hcsr04.h"

extern TIM_HandleTypeDef htim1;
void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < us);
}

void HCSR04_IC_Trigger(HCSR04_IC_t *sensor)
{
    HAL_GPIO_WritePin(sensor->trigPort, sensor->trigPin, GPIO_PIN_RESET);
    HAL_Delay(1);

    HAL_GPIO_WritePin(sensor->trigPort, sensor->trigPin, GPIO_PIN_SET);
    delay_us(10);

    HAL_GPIO_WritePin(sensor->trigPort, sensor->trigPin, GPIO_PIN_RESET);
    __HAL_TIM_ENABLE_IT(sensor->htim, sensor->channel);
}

// Init
void HCSR04_IC_Init(HCSR04_IC_t *sensor)
{

    HAL_TIM_IC_Start_IT(sensor->htim, sensor->channel);
}

// Callback xử lý capture
void HCSR04_IC_Callback(HCSR04_IC_t *sensor)
{
    if (sensor->isFirstCaptured == 0)
    {
        sensor->icVal1 = HAL_TIM_ReadCapturedValue(sensor->htim, sensor->channel);
        sensor->isFirstCaptured = 1;

        // Đổi sang bắt cạnh xuống
        __HAL_TIM_SET_CAPTUREPOLARITY(sensor->htim, sensor->channel, TIM_INPUTCHANNELPOLARITY_FALLING);
    }
    else
    {
        sensor->icVal2 = HAL_TIM_ReadCapturedValue(sensor->htim, sensor->channel);

        uint32_t diff;

        if (sensor->icVal2 > sensor->icVal1)
            diff = sensor->icVal2 - sensor->icVal1;
        else
            diff = (0xFFFF - sensor->icVal1 + sensor->icVal2);

        sensor->distance = diff / 58.0f;

        sensor->isFirstCaptured = 0;

        // Reset lại bắt cạnh lên
        __HAL_TIM_SET_CAPTUREPOLARITY(sensor->htim, sensor->channel, TIM_INPUTCHANNELPOLARITY_RISING);
        __HAL_TIM_DISABLE_IT(sensor->htim, sensor->channel);
    }
}
