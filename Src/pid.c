/*
 * pid.c
 *
 *  Created on: Sep 11, 2017
 *      Author: jose
 */

#include "pid.h"
#include "tempsensors.h"

static double max, min, Kp, Kd, Ki, pre_error, integral, mset, mpv, maxI, minI;
static double p, i, d, currentOutput;
uint32_t lastTime;

double getError() {
	return pre_error;
}
double getIntegral() {
	return integral;
}

void setupPIDFromStruct() {
	setupPID(currentPID.max, currentPID.min, currentPID.Kp, currentPID.Kd, currentPID.Ki, currentPID.minI, currentPID.maxI);
}
void setupPID(double _max, double _min, double _Kp, double _Kd, double _Ki, int16_t _minI, int16_t _maxI ) {
	max = _max;
	min = _min;
	Kp = _Kp;
	Kd = _Kd;
	Ki = _Ki;
	minI = _minI;
	maxI = _maxI;
	pre_error = 0;
	integral = 0;
}

void resetPID() {
	pre_error = 0;
	integral = 0;
}
volatile int pid_err = 0;
double calculatePID( double setpoint, double pv )
{
	mset = setpoint;
	mpv = pv;

    uint32_t tick_start = HAL_GetTick();
    double dt = (tick_start - lastTime) ;
    dt = dt / 1000;
    // Calculate error
    double error = setpoint - pv;

    // Proportional term
    double Pout = Kp * error;

    // Integral term
    integral += error * dt;
    if(integral > maxI)
    	integral = maxI;
    else if(integral < minI)
        	integral = minI;
    double Iout = Ki * integral;

    // Derivative term
    double derivative;
    	if(error == pre_error)
    		derivative = 0;
    	else
    		derivative = (error - pre_error) / dt;
    double Dout = Kd * derivative;

    // Calculate total output
    double output = Pout + Iout + Dout;
    p = Pout;
    i = Iout;
    d = Dout;
    // Restrict to max/min
    if( output > max )
        output = max;
    else if( output < min )
        output = min;

    // Save error to previous error
    pre_error = error;
    lastTime = HAL_GetTick();

    if(lastTime != tick_start){
        pid_err++;
    }
    currentOutput = output;
    return output;
}
double getPID_D() {
	return d * 100;
}
double getPID_P() {
	return p * 100;
}
double getPID_I() {
	return i * 100;
}
double getOutput() {
	return currentOutput * 100;
}
double getPID_SetPoint() {
	return mset;
}
double getPID_PresentValue() {
	return mpv;
}
