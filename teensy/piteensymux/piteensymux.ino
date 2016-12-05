//piteensymux 0.3

#include <math.h>

int r0 = 0;//value of select pin at the 4051 (s0)
int r1 = 0;//value of select pin at the 4051 (s1)
int r2 = 0;//value of select pin at the 4051 (s2)

int led = 13;

int const numAdc = 10; //number of adcs used (number of muxes)
int const numSensor = 8; // number of sensors (on each mux)
int const numReadings = 6;
float const pastWeight = 0.7;

int i, j = 0;

//2D arrays
int adcArray [numAdc][numSensor];
int adcArrayTemp [numAdc][numSensor];

//byte array
byte adcArrayByte [numAdc][numSensor];
byte adcArrayByteTemp [numAdc][numSensor];

void setup() {
	//Initialize serial coms
//	 Serial.begin(57600); //Teensy debug serial
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
}

//define MIDI output function for control change messages
void sendMidicc (int channel, int cc, float value){
	Serial3.write(char(175 + channel));
	Serial3.write(char(cc));
	Serial3.write(char(value));
};

/////////////////MAIN LOOP/////////////////
void loop(){

	for (i=0; i< numSensor; i++) {

		// bit counter
		r0 = bitRead(i,0);
		r1 = bitRead(i,1);
		r2 = bitRead(i,2);

		// write bit counter pins
		digitalWrite(2, r0);
		digitalWrite(3, r1);
		digitalWrite(4, r2);

		for (j=0; j< numAdc; j++){

			//read analog values into array
			// adcArrayTemp[j][i] = analogRead(j);
			for (int z = 0; z < numReadings; z++) {
				adcArrayTemp[j][i] = (adcArrayTemp[j][i] * pastWeight) + ((analogRead(j) * (1-pastWeight)));
			}

			adcArrayByteTemp[j][i] = (byte)(round((float)adcArrayTemp[j][i]/1024 * 250)); //reserve slip encoding bytes 253, 254, 255

			//filter  noise (if difference between stored and temp values is bigger than...)
			if ( adcArrayByteTemp[j][i] != adcArrayByte [j][i]){

				adcArrayByte[j][i] = adcArrayByteTemp [j][i];

				byte mux = j;
				byte sensor = i;
				// byte mask1 = 240;
				// byte mask2 = 15;

				//debug (needs to uncoment serial on top)
//				Serial.print(mux);
//				Serial.print("/");
//				Serial.print(sensor);
//				Serial.print("/");
//				Serial.println(adcArrayByte[j][i]);

				mux = mux << 4;

				byte id = mux | sensor;

				//send to raspberry pi with quasi 8 bit precision
				Serial2.write(253);
				Serial2.write(254);
				Serial2.write(id);
				Serial2.write(adcArrayByte[j][i]);
				Serial2.write(255);

				//send midi
				//sendMidicc(1, 60 +i, float(adcArray[0][i])/1024 *127);

//				delay(1);

			};
		};
	}
}
