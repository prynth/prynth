/*
 * This file contains function definitions of the member functions of classes:
 * 1) AnalogSensorBuf
 * 2) Filter
 */

#include "Prynth.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>

#define PI 3.141592

float abs(float a)
{
  if (a < 0)
    return -1 * a;
  return a;
}

//AnalogSensorBuf class functions
void AnalogSensorBuf::bufWrite(uint8_t sensorNum, uint8_t muxNum, int data) //add new data at the end of the buffer
{
  sensorData *temp = new sensorData;
  temp->sensor = sensorNum;
  temp->mux = muxNum;
  temp->data = data;
  temp->next = NULL;
  
  if(bufBegin == NULL)
  {
    bufBegin = temp;
    bufEnd = temp;
  }
  else
  {
    bufEnd->next = temp;
    bufEnd = temp;
  }
}

void AnalogSensorBuf::bufRead(uint8_t *sensorNum, uint8_t *muxNum, int *data) //read from start of the buffer
{
  sensorData *temp = bufBegin;
  bufBegin = bufBegin->next;
  *sensorNum = temp->sensor;
  *muxNum = temp->mux;
  *data = temp->data;
  delete(temp);
}

bool AnalogSensorBuf::bufAvailable()
{
  if(bufBegin == NULL)
    return 0;
  else
    return 1;
}


//Filter class functions
float Filter::oneEuroFilter() //1-euro filter with a specified fmin and beta
{
  float diff = (rawVal - prevRawVal) * sampRate;
  filtDiff = LPF(diff, filtDiff, dFiltCutoff);
  prevRawVal = rawVal;
  float filtCutoff = fmin + beta * abs(filtDiff);
  return LPF(rawVal, filtVal, filtCutoff);
}

float Filter::LPF(float in, float prevOut, float cutoff) //Simple Lowpass Filter of a specified cutoff frequency
{
  float alpha = 1 / (1 + sampRate/(2 * PI * cutoff));
  return (alpha*in + (1 - alpha)*(prevOut));
}


float Filter::getFilteredOutput() //Function that returns filtered output
{
  if (filtType == 1)
    filtVal = LPF(rawVal, filtVal, fmin);
  else if (filtType == 2)
    filtVal = oneEuroFilter();
  else 
  {
    prevRawVal = rawVal;
    return rawVal;
  }
return filtVal;
}

float Filter::getRawValue() //Function that passes raw value to the object
{
  return rawVal;
}

void Filter::setRawValue(int rVal)
{
  rawVal = rVal;
}  

void Filter::setFilterParam(int type = 0 , float freq = 0, float b = 0, float dCutoff = 0) //Function to set filter parameters
{
  filtType = type;
  beta = b;
  fmin = freq;
  dFiltCutoff = dCutoff;
  prevRawVal = 0;
  rawVal = 0;
  filtVal = 0;
  filtDiff = 0;
}

