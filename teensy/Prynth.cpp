/*
 * This file contains function definitions of the member functions of classes:
 * 1) SensorBuffer
 * 2) Filter
 * 3) Prynth Management Functions
 */
#include <Arduino.h>
#include "Prynth.h"
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <HardwareSerial.h>
#include "ADC.h"
#include <IntervalTimer.h>

extern float sampleFrequency;
extern int bufFullCount;

//SensorBuffer class functions
void SensorBuffer::bufWrite(uint8_t sensorNum, uint8_t muxNum, float data) //add new data at the end of the buffer
{
  if (nElements >= bufLen)
  {
    Serial.println("Buffer full");
    bufFullCount++;
    return;
  }


  sensorData *temp = new sensorData;
  temp->sensor = sensorNum;
  temp->mux = muxNum;
  temp->data = data;
  temp->next = NULL;
  nElements++;

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

void SensorBuffer::bufRead(uint8_t *sensorNum, uint8_t *muxNum, float *data) //read from start of the buffer
{
  sensorData *temp = bufBegin;
  bufBegin = bufBegin->next;
  *sensorNum = temp->sensor;
  *muxNum = temp->mux;
  *data = temp->data;
  delete(temp);
  nElements--;
}

void SensorBuffer::bufClear()
{
  sensorData *temp = bufBegin;
  while(temp != NULL)
  {
    bufBegin = bufBegin->next;
    delete(temp);
    temp = bufBegin;
  }
}

bool SensorBuffer::bufAvailable()
{
  if(bufBegin == NULL)
    return 0;
  else
    return 1;
}


//Filter class functions
float Filter::oneEuroFilter() //1-euro filter with a specified fmin and beta
{
  float diff = (rawVal - prevRawVal) * sampleFrequency;
  filtDiff = LPF(diff, filtDiff, dFiltCutoff);
  prevRawVal = rawVal;
  float filtCutoff = fmin + beta * abs(filtDiff);
  return LPF(rawVal, filtVal, filtCutoff);
}

float Filter::LPF(float in, float prevOut, float cutoff) //Simple Lowpass Filter of a specified cutoff frequency
{
  float alpha = 1 / (1 + sampleFrequency/(2 * PI * cutoff));
  return (alpha*in + (1 - alpha)*(prevOut));
}

float Filter::getFilteredOutput() //Function that returns filtered output
{
  if (filtType == 1)
    filtVal = LPF(rawVal, filtVal, fmin);
  else if (filtType == 2)
    filtVal = oneEuroFilter();
  else if (filtType == 3)
  {
    if (rawVal > 0.6)
      filtVal = 1;
    else if(rawVal < 0.4)
      filtVal = 0;
  }
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

void Filter::setRawValue(float rVal)
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


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Prynth Management Functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ADC *adc = new ADC(); // adc object
int adcResolution = 12;
IntervalTimer timer0; // Interval Timer for setting sampling rate
int startTimerValue0 = 0;
float period;
int readPeriod;

int r0 = 0;//value of select pin at the 4051 (s0)
int r1 = 0;//value of select pin at the 4051 (s1)
int r2 = 0;//value of select pin at the 4051 (s2)

int sensorCount = 0;
uint8_t readSensor = 0, readMux = 0;

int sensorValueRead[16][8] = {0};
int sensorValueReadPrev[16][8] = {0};

extern SensorBuffer sensorBuf;
extern Filter sensorFilter[16][8];
extern bool sensorResolution[16][8];
extern bool sensorActive[16][8];
extern int numAdc;
extern int numSensor;
extern bool collectSensorData;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void timer0_callback(void)
{

  if(collectSensorData == HIGH)
  {
    do
    {
      readSensor = (readSensor + 1) % numSensor;
      if(readSensor == 0)
      {
        if (readMux + 1 > numAdc)
        {
          readMux = (readMux + 1) % numAdc;
          return;
        }
        readMux = (readMux + 1) % numAdc;
      }
    }while(sensorActive[readMux][readSensor]!=HIGH);
//    Serial.print(" Reading: ");
//    Serial.print(readMux);
//    Serial.print("\t");
//    Serial.println(readSensor);
    sensorReadInit();
  }
}

void sensorReadInit()
{
  if (readMux >= 10)
    digitalSensorRead(readMux, readSensor);
  else
  {
    // bit counter
    r0 = bitRead(readSensor,0);
    r1 = bitRead(readSensor,1);
    r2 = bitRead(readSensor,2);
    digitalWriteFast(2, (bool)r0);
    digitalWriteFast(3, (bool)r1);
    digitalWriteFast(4, (bool)r2);

    adc->startSingleRead(readMux+14, ADC_0); // also: startSingleDifferential, analogSynchronizedRead, analogSynchronizedReadDifferential
  }
}

void adc0_isr()
{
    int adcReadValue = adc->readSingle();
    if (adcReadValue != sensorValueReadPrev[readMux][readSensor])
    {
      sensorBuf.bufWrite(readSensor, readMux, adcReadValue);
      sensorValueReadPrev[readMux][readSensor] = adcReadValue;
    }

    // restore ADC config if it was in use before being interrupted by the analog timer
    if (adc->adc0->adcWasInUse)
    {
        // restore ADC config, and restart conversion
        adc->adc0->loadConfig(&adc->adc0->adc_config);
        // avoid a conversion started by this isr to repeat itself
        adc->adc0->adcWasInUse = false;
    }
}


void serialSend(byte packet[], byte len) //perform SLIP encode and send packet
{
  Serial1.write("\\$");
  for(int i = 0; i < len; i++)
  {
    if (packet[i] == '\\')
    {
      Serial1.write("\\");
    }
    Serial1.write(packet[i]);
  }
  Serial1.write("\\*");
}

void send2Rpi(bool res, byte mux, byte sensor, float value)
{
  byte id = ((byte)res<<7) | (mux << 3) | (sensor);  // 8-bit id: res(7)-mux(3-6)-sensor(0-2)
  if (res == 0) //low resolution - sends 8-bit value
  {
    byte packet[3] = {0};
    packet[0] = id;
    packet[1] = (byte)value;
    packet[2] = packet[0]^packet[1];  //checksum
    serialSend(packet, 3);
  }
  else //high resolution - sends floating-point value
  {
    unsigned long f2l = *(unsigned long*)&value;
    byte packet[6] = {0};
    packet[0] = id;
    packet[1] = (byte)((f2l >> 24) & 0x000000FF);
    packet[2] = (byte)((f2l >> 16) & 0x000000FF);
    packet[3] = (byte)((f2l >> 8) & 0x000000FF);
    packet[4] = (byte)(f2l & 0x000000FF);
    for (int i = 0; i < 5; i++) //checksum
      packet[5] ^= packet[i];
    serialSend(packet, 6);
  }
}

float byte2float(byte b[]) //take an array of 4 bytes and returns the float equivalent
{
  unsigned long l = 0;
  float l2f;
  l |= ((unsigned long)b[0] << 24) & 0xFF000000;
  l |= ((unsigned long)b[1] << 16) & 0x00FF0000;
  l |= ((unsigned long)b[2] << 8) & 0x0000FF00;
  l |= ((unsigned long)b[3]) & 0x000000FF;
  l2f = *(float*)&l;
  return l2f;
}

void parseData(byte* data, byte len)
{
  char checksum = 0;
  for(int i = 0; i < len; i++) //compute checksum
  {
    checksum ^= data[i];
  }
  if (checksum != 0) //send negative acknowledgement
  {
    byte neg[1];
    neg[0] = data[1] ^0x80;
    serialSend(neg, 1);
    return;
  }
  // checksum passed
  byte pos[1];
  pos[0] = data[1];
  serialSend(pos, 1); //send positive acknowledgement

  /*
   * Types of messages that can be received from the raspberry pi:
   * 1) Change Sample Rate: (data[0]&0xF0) == 0x00
   * 2) Set Filter Parameters: (data[0]&0xF0) == 0x10
   * 3) Update sensorActive: (data[0]&0xF0) == 0x20
   * 4) Set Sensor Resolution: (data[0]&0xF0) == 0x30
   */
  if((data[0]&0xF0) == 0x00)  //change sample rate
  {
    sampleFrequency = (float)byte2float(&data[1]);
    period = pow(10,6) / (sensorCount * sampleFrequency);
    readPeriod = (int)period;
    if ((sampleFrequency <= 0 || sensorCount <= 0) && (startTimerValue0 != 0))
    {
      timer0.end();
      startTimerValue0 = 0;
    }
    else
      startTimerValue0 = timer0.begin(timer0_callback, readPeriod);
  }

  else if((data[0]&0xF0) == 0x10)  //set filter parameters
  {
//    bool res = ((data[1] & 0x80) == 0x80);
    byte mux = (data[1]>>3) & 0x0F;
    byte sensor = data[1] & 0x07;

    byte filtType = data[0]&0x0F;
    float fCutoff = byte2float(&data[2]);
    float beta = byte2float(&data[6]);
    float diffCutoff = byte2float(&data[10]);

//    sensorResolution[mux][sensor] = res;
    sensorFilter[mux][sensor].setFilterParam(filtType, fCutoff, beta, diffCutoff);
  }

  else if ((data[0]&0xF0) == 0x20)  //Update Sensor Mask
  {
    bool mask = ((data[1] & 0x80) == 0x80);
    byte mux = (data[1]>>3) & 0x0F;
    byte sensor = data[1] & 0x07;


    byte newSensorCount = sensorCount;
    if (sensorActive[mux][sensor] != mask)
    {
      sensorActive[mux][sensor] = mask;
      newSensorCount = ((mask == 1)? (sensorCount + 1) : (sensorCount - 1) );
      if ((newSensorCount == 0) && (startTimerValue0 != 0))
      {
        timer0.end();
        startTimerValue0 = 0;
      }
      else if(newSensorCount > 0)
      {
        period = pow(10,6) / (newSensorCount * sampleFrequency);
        readPeriod = (int)period;
        startTimerValue0 = timer0.begin(timer0_callback, readPeriod);
      }
      sensorCount = newSensorCount;
    }
  }

  else if((data[0]&0xF0) == 0x30)  //Set Sensor Resolution
  {
    bool res = ((data[1] & 0x80) == 0x80);
    byte mux = (data[1]>>3) & 0x0F;
    byte sensor = data[1] & 0x07;

    sensorResolution[mux][sensor] = res;
  }

  else if((data[0]&0xF0) == 0x40)  //Reset All Sensors.
  {
    for(int i = 0; i<numAdc; i++)
    {
      for(int j=0; j<numSensor; j++)
      {
        sensorResolution[i][j] = LOW;
        sensorActive[i][j] = LOW;
        sensorFilter[i][j].setFilterParam(0, 0, 0, 0);
      }
    }
    sensorCount = 0;
    sampleFrequency = 10;
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  //use if-condition to check for other types of messages sent by RPi (for future use)



  ///////////////////////////////////////////////////////////////////////////////////////////////////////
}

byte inCount = 0;
byte esc = 0;
byte inData[20] = {0};
void getSerial() //Receive serial data and SLIP decode
{
  while(Serial1.available() != 0)
  {
    char inByte = Serial1.read();
    if(inByte == '\\')
    {
      if(esc == 1)
       {
        inData[inCount++] = '\\';
        esc = 0;
       }
       else
       {
        esc = 1;
       }
    }
    else
    {
      if(esc == 1)
      {
        if(inByte == '$')
        {
          inCount = 0;
          esc = 0;
        }
        else if(inByte == '*')
        {
          parseData(inData, inCount);
          esc = 0;
          return;
        }
      }
      else
      {
        inData[inCount++] = inByte;
      }
    }
  }
}

void prynthInit()
{

  //mux control pins initialization
  pinMode(2, OUTPUT);// s0
  pinMode(3, OUTPUT);// s1
  pinMode(4, OUTPUT);// s2

  //ADC Init
  adc->setAveraging(1); // set number of averages
  adc->setResolution(adcResolution); // set bits of resolution

  adc->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_HIGH_SPEED); // change the conversion speed
  adc->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); // change the sampling speed

  digitalSensorInit();
  // Initialize all sensors to Low resolution and set their masks.
  for(int i = 0; i<numAdc; i++)
  {
    for(int j=0; j<numSensor; j++)
    {
      sensorResolution[i][j] = LOW;
      sensorActive[i][j] = LOW;
    }
  }

  period = pow(10,6) / (sensorCount*sampleFrequency) ;
  readPeriod = (int)period; // us
  Serial.println("Starting Timer");
  // start the timers, if it's not possible, startTimerValuex will be false
  startTimerValue0 = timer0.begin(timer0_callback, readPeriod);
  // wait enough time for the first timer conversion to finish (depends on resolution and averaging),
  // with 16 averages, 12 bits, and ADC_MED_SPEED in both sampling and conversion speeds it takes about 36 us.
  delayMicroseconds(25); // if we wait less than 36us the timer1 will interrupt the conversion
  Serial.println("Timer started");

  adc->enableInterrupts(ADC_0);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
