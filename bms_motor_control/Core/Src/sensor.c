/*
 * sensor.c
 *
 *  Created on: Apr 20, 2026
 *      Author: Phong
 */

#include "sensor.h"

double A_s =  0.00089065;
double B_s =  0.000251;
double C_s = 0.000000194;

/*
 * @brief  This function will convert Resistance which measure through voltage, to temperature
 */
double Steinhart_Hart(double R)
{
	R = R * 1000;
	double T_K = 1.0 / (A_s + B_s*log(R) + C_s * pow(log(R) , 3));
	double T_C = T_K - 273.15;
	return T_C;
}

/*
 * @Brief	This function will convert measured voltage to Resistance
 */
double VoltageToResistance(double voltage)
{
	voltage = voltage / 1000;
	double Resistance = 2.0 / (1.0/voltage -0.2);
	return Resistance;
}

