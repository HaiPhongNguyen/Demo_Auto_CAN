/*
 * sensor.h
 *
 *  Created on: Apr 20, 2026
 *      Author: Phong
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#include "main.h"

HAL_StatusTypeDef ADC1_Channel_Selection(ADC_ChannelConfTypeDef* sConfig)
{
	HAL_ADC_ConfigChannel
}
/*
 * @brief  This function will convert Resistance which measure through voltage, to temperature
 */
double Steinhart_Hart(double R);

/*
 * @Brief	This function will convert measured voltage to Resistance
 */
double VoltageToResistance(double voltage);

#endif /* INC_SENSOR_H_ */
