#ifndef PRYNTH_H
#define PRYNTH_H


#include<stddef.h>
#include<stdint.h>

/*
 * Class to hold sensor data read at a constant rate
 * Implemented as a queue of struct sensorData using linked lists
 * sensorData contains value of sensor along with the sensor number and mux it is connected to.
 */

class AnalogSensorBuf{
private:
  struct sensorData
  {
    uint8_t sensor, mux;
    int data;
    sensorData* next;
  }*bufBegin, *bufEnd;
public:
  void bufWrite(uint8_t sensorNum, uint8_t muxNum, int data);
  void bufRead(uint8_t *sensorNum, uint8_t *muxNum, int *data);
  bool bufAvailable();
};

/*
 * Class to perform filtering of sensor data
 * Filter types available: 
 *    0 - No Filter
 *    1 - 2nd Order Lowpass Filter
 *    2 - 1-euro Filter (Add more?)
 * Contains functions to output raw data and filtered output. 
 * Contains function to set filter parameters.
 */

class Filter{
private:
  int rawVal;
  int prevRawVal;
  float filtDiff; //filtered differential signal
  float filtVal;  //filtered output  
  int filtType; //NoFilter - 0, LPF - 1, 1Euro - 2
  float beta; //for 1-euro filter
  float fmin; //also the cutoff for LPF
  float dFiltCutoff;
  int sampRate;

  float oneEuroFilter();
  float LPF(float in, float prevOut, float cutoff);

public:
  Filter(int sRate = 10, int type = 0, float freq = 1.0, float b = 0, float dCutoff = 1.0)
  {
    sampRate = sRate;
    filtType = type;
    beta = b;
    fmin = freq;
    dFiltCutoff = dCutoff;
    prevRawVal = 0;
    rawVal = 0;
    filtVal = 0;
    filtDiff = 0;
  }
  float getFilteredOutput();
  float getRawValue();
  void setRawValue(int rVal);
  void setFilterParam(int type, float freq, float b, float dCutOff);
};
  
#endif // PRYNTH_H
