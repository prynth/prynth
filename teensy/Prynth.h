#ifndef PRYNTH_H
#define PRYNTH_H


#include<stddef.h>
#include<stdint.h>


/*
 * Class to hold sensor data read at a constant rate
 * Implemented as a queue of struct sensorData using linked lists
 * sensorData contains value of sensor along with the sensor number and mux it is connected to.
 */
class SensorBuffer{
private:
  struct sensorData
  {
    uint8_t sensor, mux;
    float data;
    sensorData* next;
  }*bufBegin, *bufEnd;
  int bufLen;
  int nElements;
public:
  SensorBuffer(int len)
  {
    bufLen = len;
    nElements = 0;
  }
  void bufWrite(uint8_t sensorNum, uint8_t muxNum, float data);
  void bufRead(uint8_t *sensorNum, uint8_t *muxNum, float *data);
  void bufClear();
  bool bufAvailable();
  int getNElements()
  {
    return nElements;
  }
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

/*
 * Insert credit to 1-euro filter here.
 */
class Filter{
private:
  float rawVal;
  float prevRawVal;
  float filtDiff; //filtered differential signal
  float filtVal;  //filtered output
  int filtType; //NoFilter - 0, LPF - 1, 1Euro - 2, Schmitt Trigger - 3
  float beta; //for 1-euro filter
  float fmin; //also the cutoff for LPF
  float dFiltCutoff;

  float oneEuroFilter();
  float LPF(float in, float prevOut, float cutoff);

public:
  Filter(int type = 0, float freq = 1.0, float b = 0, float dCutoff = 1.0)
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
  float getFilteredOutput();
  float getRawValue();
  void setRawValue(float rVal);
  void setFilterParam(int type, float freq, float b, float dCutOff);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prynth Management Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void timer0_callback(void);
void sensorReadInit();
void digitalSensorInit();
void digitalSensorRead(int mux, int sensor);
void adc0_isr();
void send2Rpi(bool res, byte mux, byte sensor, float value);
void serialSend(byte packet[], byte len);
float byte2float(byte b[]);
void parseData(byte* data, byte len);
void getSerial();
void prynthInit();

#endif // PRYNTH_H
