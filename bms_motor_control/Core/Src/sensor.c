/*
 * sensor.c
 *
 *  Created on: Apr 20, 2026
 *      Author: Phong
 */

#include "sensor.h"
#include "math.h"

extern ADC_HandleTypeDef hadc1;

double A_s =  0.00089065;
double B_s =  0.000251;
double C_s = 0.000000194;

/********** Local function definition *********/
static HAL_StatusTypeDef ADC1_Channel_Selection(uint32_t channel);
static double Steinhart_Hart(double R);
static double VoltageToResistance(double voltage);

/*
 * @brief  This function will convert Resistance which measure through voltage, to temperature
 */
static double Steinhart_Hart(double R)
{
	R = R * 1000;
	double T_K = 1.0 / (A_s + B_s*log(R) + C_s * pow(log(R) , 3));
	double T_C = T_K - 273.15;
	return T_C;
}

/*
 * @Brief	This function will convert measured voltage to Resistance
 */
static double VoltageToResistance(double voltage)
{
	double Resistance = 10.0 / (3.333/voltage - 1);
	return Resistance;
}

static HAL_StatusTypeDef ADC1_Channel_Selection(uint32_t channel)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;

	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		return HAL_ERROR;
	}
	return HAL_OK;
}

float readTemperature()
{
	uint32_t sum_adcvalue = 0;
	uint16_t adc_value = 0x0000;
	double voltage;
	double resistance;

	ADC1_Channel_Selection(ADC_CHANNEL_1);
	for(uint8_t index = 0; index < 100; index++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 100);
		sum_adcvalue += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
	}

	adc_value = sum_adcvalue / 100;
	voltage = (adc_value * 3.3333) / 4095;

	/* calculate resistance */
	resistance = VoltageToResistance(voltage);
	return Steinhart_Hart(resistance);
}

float readVbat()
{
	uint32_t sum_adcvalue = 0;
	uint16_t adc_value = 0x0000;
	double voltage;

	ADC1_Channel_Selection(ADC_CHANNEL_9);
	for(uint8_t index = 0; index < 100; index++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 100);
		sum_adcvalue += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
	}

	adc_value = sum_adcvalue / 100;
	voltage = (adc_value * 3.30) / 4095;

	return (double)((voltage / R2) * (R1 + R2));

}

float readCurrent(float offset)
{
	uint32_t sum_adcvalue = 0;
	uint16_t adc_value = 0x0000;
	double voltage;
	double adc_voltage;

	ADC1_Channel_Selection(ADC_CHANNEL_8);
	for(uint8_t index = 0; index < 100; index++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 100);
		sum_adcvalue += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
	}

	adc_value = sum_adcvalue / 100;
	voltage = (adc_value * 3.333) / 4095;

	adc_voltage = voltage * 2.088;

	return (adc_voltage - offset) / 0.1;
}

float readOffset()
{
	uint32_t sum_adcvalue = 0;
	uint16_t adc_value = 0x0000;
	double voltage;
	double adc_voltage;

	ADC1_Channel_Selection(ADC_CHANNEL_8);
	for(uint8_t index = 0; index < 200; index++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 100);
		sum_adcvalue += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
	}

	adc_value = sum_adcvalue / 200;
	voltage = (adc_value * 3.333) / 4095;

	adc_voltage = voltage * 2.088;

	return adc_voltage;
}
