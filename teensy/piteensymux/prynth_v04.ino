// Prynth v04

#include "ADC.h"
#include <math.h>
#include "Prynth.h"
#include <IntervalTimer.h>

int r0 = 0;//value of select pin at the 4051 (s0)
int r1 = 0;//value of select pin at the 4051 (s1)
int r2 = 0;//value of select pin at the 4051 (s2)

int led = 13;

int const numAdc = 3; //number of adcs used (number of muxes)
int const numSensor = 8; // number of sensors (on each mux)
int const numReadings = 1;
float const pastWeight = 0.9;

////2D arrays
//int adcArray [numAdc][numSensor];
//int adcArrayTemp [numAdc][numSensor];

//byte array
byte adcArrayByte [numAdc][numSensor];
byte adcArrayByteTemp [numAdc][numSensor];

//Set sampling frequency for sensors (same sampling rate for all sensors)
int readFrequency = 10; //Hz - Enter the rate at which the sensors need to be sampled. Note: All sensors will be sampled at the same rate.
float period = pow(10,6) / (numAdc*numSensor*readFrequency) ;
const int readPeriod = (int)period; // us

ADC *adc = new ADC(); // adc object

//ADC buffer for constant rate sampling 
AnalogSensorBuf sensorBuf;

//Filters for sensors
//No filtering by default - use setFilterParam() to set the filtering type and parameters in setup()
Filter sensorFilter[numAdc][numSensor] = {Filter(readFrequency,0,0,0,0)};

//Interval Timer for setting sampling rate
IntervalTimer timer0; // timers
int startTimerValue0 = 0;

uint8_t readSensor = 0, readMux = 0;

void setup() {
  //Initialize serial coms
  Serial.begin(9600); //Teensy USB debug serial
  Serial2.begin(57600, SERIAL_8N1);//Pi serial
  //Serial3.begin(31250); //MIDI serial

  //led pin output mode
  pinMode(led, OUTPUT);

  //mux control pins initialization
  pinMode(2, OUTPUT);// s0
  pinMode(3, OUTPUT);// s1
  pinMode(4, OUTPUT);// s2

  //turn led on
  digitalWrite(led, HIGH);

  //ADC Init
  adc->setAveraging(1); // set number of averages
  adc->setResolution(12); // set bits of resolution

  Serial.print("Read Period: ");
  Serial.print(readPeriod);
  Serial.println("microseconds");

  // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED_16BITS, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
  adc->setConversionSpeed(ADC_MED_SPEED); // change the conversion speed
  // it can be ADC_VERY_LOW_SPEED, ADC_LOW_SPEED, ADC_MED_SPEED, ADC_HIGH_SPEED or ADC_VERY_HIGH_SPEED
  adc->setSamplingSpeed(ADC_MED_SPEED); // change the sampling speed
  
  Serial.println("Starting Timers");

  // start the timers, if it's not possible, startTimerValuex will be false
  startTimerValue0 = timer0.begin(timer0_callback, readPeriod);
  // wait enough time for the first timer conversion to finish (depends on resolution and averaging),
  // with 16 averages, 12 bits, and ADC_MED_SPEED in both sampling and conversion speeds it takes about 36 us.
  delayMicroseconds(25); // if we wait less than 36us the timer1 will interrupt the conversion

  adc->enableInterrupts(ADC_0);
  Serial.println("Timers started");

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialize filters here
  // Filter types available:
  // 0 - No filter
  // 1 - Low Pass Filter (Only enter filter_type and filter_cutoff. Enter 0 for 1euro_beta_value and difference_signal_cutoff)
  // 2 - One-Euro Filter 
  // sensorFilter[0][0].setFilterParam(filter_type, filter_cutoff, 1euro_beta_value, difference_signal_cutoff);




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  delay(500);
  
}

//define MIDI output function for control change messages
void sendMidicc (int channel, int cc, float value){
  Serial3.write(char(175 + channel));
  Serial3.write(char(cc));
  Serial3.write(char(value));
}

void send2Rpi (byte id, byte value)
{
  Serial2.write(253);
  Serial2.write(254);
  Serial2.write(id);
  Serial2.write(value);
  Serial2.write(255);
}


/////////////////MAIN LOOP/////////////////
void loop(){
  
  uint8_t sNum, mNum;
  int sVal;
  
  if(sensorBuf.bufAvailable() == 1)
  {
    //Read values from the buffer
    sensorBuf.bufRead(&sNum, &mNum, &sVal);
    sensorFilter[mNum][sNum].setRawValue(sVal);
    
    //encode to quasi-8bit resolution, reserving slip encoding bytes 253, 254, 255
    adcArrayByteTemp[mNum][sNum] = (byte)(round((float)sensorFilter[mNum][sNum].getFilteredOutput()/4096 * 250));

    //if it's different from past values send via serial to RPi
    if ( adcArrayByteTemp[mNum][sNum] != adcArrayByte [mNum][sNum])
    {
      adcArrayByte[mNum][sNum] = adcArrayByteTemp [mNum][sNum];
      
      // byte mask1 = 240;
      // byte mask2 = 15;

      //debug (needs to uncoment USB serial on top)
      //Serial.print(mux);
      //Serial.print("/");
      //Serial.print(sensor);
      //Serial.print("/");
      //Serial.println(adcArrayByte[j][i]);
      
      byte id = (mNum<<4) | sNum;

      // send to raspberry pi
      send2Rpi(id, adcArrayByte[mNum][sNum]);

      //send if midi serial enabled
      //sendMidicc(1, 60 +sNum, float(adcArray[0][sNum])/1024 *127);
    }
  }
}


void timer0_callback(void) 
{
  // Measure Time taken to execute these lines
//  Serial.println("timer");
  readSensor = (readSensor + 1) % numSensor;
  if(readSensor == 0)
    readMux = (readMux + 1) % numAdc;
  
  // bit counter
  r0 = bitRead(readSensor,0);
  r1 = bitRead(readSensor,1);
  r2 = bitRead(readSensor,2);
  digitalWriteFast(2, r0);
  digitalWriteFast(3, r1);
  digitalWriteFast(4, r2);
  
  adc->startSingleRead(readMux+14, ADC_0); // also: startSingleDifferential, analogSynchronizedRead, analogSynchronizedReadDifferential
}

void adc0_isr() 
{
//    uint8_t pin = ADC::sc1a2channelADC0[ADC0_SC1A&ADC_SC1A_CHANNELS]; // the bits 0-4 of ADC0_SC1A have the channel
//    Serial.println("adc");
    int adcReadValue = adc->readSingle();
    sensorBuf.bufWrite(readSensor, readMux, adcReadValue);
    
    // restore ADC config if it was in use before being interrupted by the analog timer
    if (adc->adc0->adcWasInUse) 
    {
        // restore ADC config, and restart conversion
        adc->adc0->loadConfig(&adc->adc0->adc_config);
        // avoid a conversion started by this isr to repeat itself
        adc->adc0->adcWasInUse = false;
    }
}

  
