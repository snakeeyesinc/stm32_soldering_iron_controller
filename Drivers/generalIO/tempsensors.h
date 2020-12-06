/*
 * tempsensors.h
 *
 *  Created on: Jul 21, 2017
 *      Author: jose
 */

#ifndef GENERALIO_TEMPSENSORS_H_
#define GENERALIO_TEMPSENSORS_H_

#include "main.h"
#include "adc_global.h"
#include "settings.h"
#include "iron.h"


#define New 1
#define Old 0
#define toCelsius	0
#define toFarenheit	1
int16_t  readColdJunctionSensorTemp_C_x10(void);
uint16_t coldJunctionTemp_mC_To_uV(int tempX10);
uint16_t readIntTemp_mC(void);
uint16_t readTipSensorADC_Avg(void);
int16_t readColdJunctionSensorTemp_x10(bool tempUnit);
uint16_t readTipTemperatureCompensated(bool new);
uint16_t readTipTemperatureCompensatedRaw(bool new);
uint16_t realTempToADC(uint16_t real);
void setCurrentTip(uint8_t tip);
tipData * getCurrentTip();

long map(long x, long in_min, long in_max, long out_min, long out_max);
int16_t adc2Human(uint16_t adc_value,bool correction, bool tempUnit);
uint16_t human2adc(int16_t t);
int16_t TempConversion(int16_t temperature, bool conversion);






#endif /* GENERALIO_TEMPSENSORS_H_ */

