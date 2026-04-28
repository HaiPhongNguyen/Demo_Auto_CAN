/*
 * sensor.h
 *
 *  Created on: Apr 20, 2026
 *      Author: Phong
 */

#ifndef INC_SENSOR_H_
#define INC_SENSOR_H_

#include "main.h"

#define R1 10
#define R2 2.5

float readTemperature();
float readVbat();
float readCurrent(float offset);
float readOffset();


#endif /* INC_SENSOR_H_ */
