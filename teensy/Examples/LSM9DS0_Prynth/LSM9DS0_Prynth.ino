// Prynth v05

#include "ADC.h"
#include <math.h>
#include "Prynth.h"
#include<i2c_t3.h>
#include "SFE_LSM9DS0.h"

int led = 13;

int numAdc = 16; //number of adcs used (number of muxes)
int numSensor = 8; // number of sensors (on each mux)

bool collectSensorData = LOW;

//sensor data arrays
float sensorArray[16][8];
float sensorArrayTemp[16][8];
bool sensorResolution[16][8] = {0};
bool sensorActive[16][8]= {0};

// Set sampling frequency in Hz . Each and every sensor will be sampled at this same rate.
float sampleFrequency = 10;

//Sensor Buffer initialization
SensorBuffer sensorBuf(10000);

// Filters for sensors
// No filtering by default - use setFilterParam() to set the filtering type and parameters in setup()
Filter sensorFilter[16][8];

int bufFullCount = 0;

void setup()
{
	//Initialize serial coms
	Serial.begin(57600); // initialize this if you need to debug teensy via USB
	Serial1.begin(3000000, SERIAL_8N1); //Teensy to RPi
	//Serial3.begin(31250); //MIDI serial

	prynthInit();

	//led pin output mode
	pinMode(led, OUTPUT);
	//turn led on
	digitalWrite(led, HIGH);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Initialize sensors here.
	// By default, all sensors are of LOW resolution and no filtering is applied.
	//
	// Sensor Resolution:
	// LOW - Range: 0-255
	// HIGH - Range: range of float
	// Example: sensorResolution[0][0] = HIGH;
	//
	// Available Filter types:
	// 0 - No filter
	// 1 - Low Pass Filter (Only enter filter_type and filter_cutoff. Enter 0 for 1euro_beta_value and difference_signal_cutoff)
	// 2 - One-Euro Filter (One Euro Filter by Géry Casiez, Nicolas Roussel and Daniel Vogel. More info: http://cristal.univ-lille.fr/~casiez/1euro/).

	// Example: sensorFilter[0][0].setFilterParam(filter_type, filter_cutoff, 1euro_beta_value, difference_signal_cutoff);

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Setup complete, start collecting sensor data
	collectSensorData = true;
}

//Function to send MIDI control change messages via Serial3
void sendMidicc (int channel, int cc, float value){
	Serial3.write(char(175 + channel));
	Serial3.write(char(cc));
	Serial3.write(char(value));
}

/////////////////MAIN LOOP/////////////////
void loop(){
	uint8_t sNum, mNum;
	float sVal;

	if(sensorBuf.bufAvailable() == 1)
	{
		//Read values from the buffer and filter them
		sensorBuf.bufRead(&sNum, &mNum, &sVal);
		if (mNum <= 9)  sVal /= pow(2,12);
		sensorFilter[mNum][sNum].setRawValue(sVal);

		if (sensorResolution[mNum][sNum] == HIGH) //High resolution - send float values
		{
			sensorArrayTemp[mNum][sNum] = sensorFilter[mNum][sNum].getFilteredOutput();
		}
		else  //Low resolution - send byte values
		{
			sensorArrayTemp[mNum][sNum] = round((float)sensorFilter[mNum][sNum].getFilteredOutput() * 255);
		}

		//if it's different from past values send via serial to RPi
		if (sensorArrayTemp[mNum][sNum] != sensorArray[mNum][sNum])
		{
			sensorArray[mNum][sNum] = sensorArrayTemp [mNum][sNum];

			bool res = sensorResolution[mNum][sNum];
			float sensorValue = sensorArray[mNum][sNum];

			//debug (requires uncommenting USB serial on top)
			//Serial.print(mNum);
			//Serial.print("/");
			//Serial.print(sNum);
			//Serial.print("/");
			//Serial.println(sensorArray[mNum][sNum]);

			// send to Raspberry Pi
			send2Rpi(res, mNum, sNum, sensorValue);

			//send if MIDI serial enabled
			//sendMidicc(1, 60 +sNum, float(adcArray[0][sNum])/1024 *127);
		}
	}

	// Control data from Raspberry Pi,  for setting filter parameters, sensor data resolution, sample rate, sensor mask, etc.
	if(Serial1.available())
	{
		noInterrupts();
		collectSensorData = false;  //Pause sensor data acquisition.
		digitalWrite(led, LOW);
		sensorBuf.bufClear(); //Clear previous values in the sensor data buffer.
		getSerial(); // Parse the received data and process.
		collectSensorData = true; //Resume sensor data acquisition.
		digitalWrite(led,HIGH);
		interrupts();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Digital sensor handling
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * First, the functions necessary for handling the I2C communication between the
 * sensor and the microcontroller must be added.
 * Second, initialize the sensors by calling the necessary functions in digitalSensorInit().
 * Finally, read the sensors and queue them into the sensorBuf buffer using the appropriate
 * mux and sensor values, within the digitalSensorRead() function.
 */

///////////////////////
// Example I2C Setup //
///////////////////////
// Comment out this section if you're using SPI
// SDO_XM and SDO_G are both grounded, so our addresses are:
#define LSM9DS0_XM  0x1D // Would be 0x1E if SDO_XM is LOW
#define LSM9DS0_G   0x6B // Would be 0x6A if SDO_G is LOW
// Create an instance of the LSM9DS0 library called `dof` the
// parameters for this constructor are:
// [SPI or I2C Mode declaration],[gyro I2C address],[xm I2C add.]
LSM9DS0 dof(MODE_I2C, LSM9DS0_G, LSM9DS0_XM);


// global constants for 9 DoF fusion and AHRS (Attitude and Heading Reference System)
#define GyroMeasError PI * (40.0f / 180.0f)       // gyroscope measurement error in rads/s (shown as 3 deg/s)
#define GyroMeasDrift PI * (0.0f / 180.0f)      // gyroscope measurement drift in rad/s/s (shown as 0.0 deg/s/s)
// There is a tradeoff in the beta parameter between accuracy and response speed.
// In the original Madgwick study, beta of 0.041 (corresponding to GyroMeasError of 2.7 degrees/s) was found to give optimal accuracy.
// However, with this value, the LSM9SD0 response time is about 10 seconds to a stable initial quaternion.
// Subsequent changes also require a longish lag time to a stable output, not fast enough for a quadcopter or robot car!
// By increasing beta (GyroMeasError) by about a factor of fifteen, the response time constant is reduced to ~2 sec
// I haven't noticed any reduction in solution accuracy. This is essentially the I coefficient in a PID control sense;
// the bigger the feedback coefficient, the faster the solution converges, usually at the expense of accuracy.
// In any case, this is the free parameter in the Madgwick filtering and fusion scheme.
#define beta sqrt(3.0f / 4.0f) * GyroMeasError   // compute beta
#define zeta sqrt(3.0f / 4.0f) * GyroMeasDrift   // compute zeta, the other free parameter in the Madgwick scheme usually set to a small or zero value
#define Kp 2.0f * 5.0f // these are the free parameters in the Mahony filter and fusion scheme, Kp for proportional feedback, Ki for integral
#define Ki 0.0f

uint32_t count = 0;  // used to control display output rate
uint32_t delt_t = 0; // used to control display output rate
float pitch, yaw, roll, heading;
float deltat = 0.0f;        // integration interval for both filter schemes
uint32_t lastUpdate = 0;    // used to calculate integration interval
uint32_t Now = 0;           // used to calculate integration interval

float abias[3] = {0, 0, 0}, gbias[3] = {0, 0, 0};
float ax, ay, az, gx, gy, gz, mx, my, mz; // variables to hold latest sensor data values
float q[4] = {1.0f, 0.0f, 0.0f, 0.0f};    // vector to hold quaternion
float eInt[3] = {0.0f, 0.0f, 0.0f};       // vector to hold integral error for Mahony method
float temperature;

void digitalSensorInit()
{
  /*
   * Insert code to initialize I2C and SPI sensors.
   * mux numbers from 10-15 (=6) are dedicated for digital sensors.
   * sensor numbers 0-7 (=8) are to send the sensor data (raw or cooked)
   */

  delay(500);
  Serial.println("DigitalSensorInit");
  uint32_t status = dof.begin();

  Serial.print("LSM9DS0 WHO_AM_I's returned: 0x");
  Serial.println(status, HEX);
  Serial.println("Should be 0x49D4");
  Serial.println();

//  delay(2000);

 // Set data output ranges; choose lowest ranges for maximum resolution
 // Accelerometer scale can be: A_SCALE_2G, A_SCALE_4G, A_SCALE_6G, A_SCALE_8G, or A_SCALE_16G
    dof.setAccelScale(dof.A_SCALE_2G);
 // Gyro scale can be:  G_SCALE__245, G_SCALE__500, or G_SCALE__2000DPS
    dof.setGyroScale(dof.G_SCALE_245DPS);
 // Magnetometer scale can be: M_SCALE_2GS, M_SCALE_4GS, M_SCALE_8GS, M_SCALE_12GS
    dof.setMagScale(dof.M_SCALE_2GS);

 // Set output data rates
 // Accelerometer output data rate (ODR) can be: A_ODR_3125 (3.225 Hz), A_ODR_625 (6.25 Hz), A_ODR_125 (12.5 Hz), A_ODR_25, A_ODR_50,
 //                                              A_ODR_100,  A_ODR_200, A_ODR_400, A_ODR_800, A_ODR_1600 (1600 Hz)
    dof.setAccelODR(dof.A_ODR_200); // Set accelerometer update rate at 100 Hz
 // Accelerometer anti-aliasing filter rate can be 50, 194, 362, or 763 Hz
 // Anti-aliasing acts like a low-pass filter allowing oversampling of accelerometer and rejection of high-frequency spurious noise.
 // Strategy here is to effectively oversample accelerometer at 100 Hz and use a 50 Hz anti-aliasing (low-pass) filter frequency
 // to get a smooth ~150 Hz filter update rate
    dof.setAccelABW(dof.A_ABW_50); // Choose lowest filter setting for low noise

 // Gyro output data rates can be: 95 Hz (bandwidth 12.5 or 25 Hz), 190 Hz (bandwidth 12.5, 25, 50, or 70 Hz)
 //                                 380 Hz (bandwidth 20, 25, 50, 100 Hz), or 760 Hz (bandwidth 30, 35, 50, 100 Hz)
    dof.setGyroODR(dof.G_ODR_190_BW_125);  // Set gyro update rate to 190 Hz with the smallest bandwidth for low noise

 // Magnetometer output data rate can be: 3.125 (ODR_3125), 6.25 (ODR_625), 12.5 (ODR_125), 25, 50, or 100 Hz
    dof.setMagODR(dof.M_ODR_125); // Set magnetometer to update every 80 ms

 // Use the FIFO mode to average accelerometer and gyro readings to calculate the biases, which can then be removed from
 // all subsequent measurements.
    dof.calLSM9DS0(gbias, abias);
}

long gyroTimer=0;
int gyroInterval=1;
long accelTimer=0;
int accelInterval=1;
long magTimer=0;
int magInterval=2;

#define ACC_MUX 10
#define GYR_MUX 11
#define MAG_MUX 12
#define TEMP_MUX 13

void digitalSensorRead(int mux, int sensor)
{
 /*
  *  Insert code to read data from I2C and SPI sensors.
  *  Integrate the digital sensor values into the sensorBuf queue writing the normalized values
  *  (0-1) to the buffer.
  */
  if (mux == ACC_MUX)
  {

    if(millis()>accelTimer+accelInterval) {  // When new accelerometer data is ready
      dof.readAccel();         // Read raw accelerometer data
    //  calcMagnitude();
      accelTimer=millis();
    }
    if (sensor == 0)
    {
      ax = dof.calcAccel(dof.ax) - abias[0];   // Convert to g's, remove accelerometer biases
      sensorBuf.bufWrite(sensor, mux, ax);
    }
    if (sensor == 1)
    {
      ay = dof.calcAccel(dof.ay) - abias[1];
      sensorBuf.bufWrite(sensor, mux, ay);
    }
    if (sensor == 2)
    {
      az = dof.calcAccel(dof.az) - abias[2];
      sensorBuf.bufWrite(sensor, mux, az);
    }
  }

  if (mux == GYR_MUX)
  {
    if(millis()>gyroTimer+gyroInterval) {  // When new gyro data is ready
      dof.readGyro();           // Read raw gyro data
      gyroTimer=millis();
    }
    if (sensor == 0)
    {
      gx = dof.calcGyro(dof.gx) - gbias[0];   // Convert to degrees per seconds, remove gyro biases
      sensorBuf.bufWrite(sensor, mux, gx);
    }
    if (sensor == 1)
    {
      gy = dof.calcGyro(dof.gy) - gbias[1];
      sensorBuf.bufWrite(sensor, mux, gy);
    }
    if (sensor == 2)
    {
      gz = dof.calcGyro(dof.gz) - gbias[2];
      sensorBuf.bufWrite(sensor, mux, gz);
    }
  }

  if (mux == MAG_MUX)
  {
    if(millis()>magTimer+magInterval) {  // When new magnetometer data is ready
      dof.readMag();           // Read raw magnetometer data
      magTimer = millis();
    }
    if (sensor == 0)
    {
      mx = dof.calcMag(dof.mx);     // Convert to Gauss and correct for calibration
      sensorBuf.bufWrite(sensor, mux, mx);
    }
    if (sensor == 1)
    {
      my = dof.calcMag(dof.my);
      sensorBuf.bufWrite(sensor, mux, my);
    }
    if (sensor == 2)
    {
      mz = dof.calcMag(dof.mz);
      sensorBuf.bufWrite(sensor, mux, mz);
    }
  }

  if (mux == TEMP_MUX)
  {
    dof.readTemp();
    temperature = 21.0 + (float) dof.temperature/8.; // slope is 8 LSB per degree C, just guessing at the intercept
    sensorBuf.bufWrite(0, mux, temperature);
  }
}
