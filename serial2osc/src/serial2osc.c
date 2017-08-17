#include <stdio.h>  //printing
#include <string.h> //string manipulation for osc
#include <errno.h> //error catching
#include <time.h> //clocks
#include <math.h> //rounding
// #include <wiringPi.h>
#include <wiringSerial.h>
#include <lo/lo.h>


//global variable initialization
char hostname[1024];

float sensorData[10][8];
float oscBuffer[10][8];

char byteData[80];

int switchMatrix[10][8] = {
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
	{1,1,1,1,1,1,1,1},
};

int switchMatrix1[10] = {1,1,0,0,0,0,0,0};

//method declaration
void getSerial (int *fd);
void getSerial2 (int *fd);
void printSensorArray ();
void sendOSC (lo_address *t);
void sendSCServer (lo_address *t);
void getHostname();

int mode; // 1 send to server, 2 send to lang



int main (int argc, char *argv[]) {


	if( argc == 2 ) {

		// printf("The argument supplied is %s\n", argv[1]);

		if ( strcmp(argv[1], "-s") == 0 ){
			mode = 1;
		};


		if ( strcmp(argv[1], "-l") == 0 ){
			mode = 2;
		};



	}
	else if( argc > 2 ) {
		printf("Too many arguments supplied.\n");
	}
	else {
		printf("One argument expected. -s for server -l for lang\n");
	}

	// variable setup
	int fd ; //serial
	lo_address oscAddressBroadcast; //osc address
	lo_address oscAddressServer; //osc address
	struct timespec timeStart, timeEnd; //clock types
	uint64_t timeDiff = 0; //time differential
	uint64_t timeTotal = 0; //in ms
	uint64_t serialTimeStamp = 0;
	uint64_t oscTimeStamp = 0;
	uint64_t clearTimeStamp = 0;
	uint64_t printTimeStamp = 0;
	uint64_t showTimeDiff;
	// divide by 1000000 for milis

	#define BILLION 1000000000L

	//set oscAddress
	oscAddressBroadcast = lo_address_new("127.0.0.1", "57120"); //osc address lang
	oscAddressServer = lo_address_new("127.0.0.1", "57110"); //osc address server

	//open serial
	if ((fd = serialOpen ("/dev/ttyAMA0", 57600)) < 0) {
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}

	getHostname();

	serialClose(fd);

	if ((fd = serialOpen ("/dev/ttyAMA0", 57600)) < 0) {
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}


   	// main loop
	while (1){
		clock_gettime(CLOCK_MONOTONIC, &timeStart); /* mark start time */

		delay (1); // has to spend some time

		if (timeTotal - serialTimeStamp >= 5) {
			getSerial(&fd);
			serialTimeStamp = timeTotal;
		}

		if (timeTotal - oscTimeStamp >= 10) {
			if(mode == 1){
				sendSCServer(&oscAddressServer);
			}

			if(mode == 2){
				sendOSC(&oscAddressBroadcast);
				// printf("%i\n", mode);
			}

			oscTimeStamp = timeTotal;
		}

		// print stuff
		// if (timeTotal - printTimeStamp >= 400) {
		// 	printSensorArray();
		// 	printTimeStamp = timeTotal;
		// }

		clock_gettime(CLOCK_MONOTONIC, &timeEnd); /* mark end time */
		timeDiff = (BILLION * (timeEnd.tv_sec - timeStart.tv_sec) + timeEnd.tv_nsec - timeStart.tv_nsec)/1000000;
		timeTotal = (timeTotal + timeDiff);
		showTimeDiff = timeDiff;
		// printf("%llu\n", (long long unsigned int) showTimeDiff);
		// printf("total time = %llu miliseconds\n", (long long unsigned int) showTimeDiff);
	}
	return 0 ;
}

void sendOSC (lo_address *oscAddress) {
	char string[20];
	char string2[1025];
	int i, j;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 8; j++) {
			// if (sensorData[i][j] != oscBuffer[i][j] && switchMatrix[i][j] == 1){
			// 	sprintf(string, "/%i/%i", i, j);
			// 	sprintf(string2, "/%s", hostname);
			// 	// printf("%s\n", string);
			// 	lo_send(*oscAddress, string2, "sf", string, oscBuffer[i][j]);
			// 	oscBuffer[i][j] = sensorData[i][j];
			// }
			if (sensorData[i][j] != oscBuffer[i][j] && switchMatrix[i][j] == 1){
				sprintf(string, "/%s/%i/%i", hostname, i, j);
				// sprintf(string2, "/%s", hostname);
				// printf("%s\n", string);
				lo_send(*oscAddress, string, "f", oscBuffer[i][j]);
				oscBuffer[i][j] = sensorData[i][j];
			}
		}
	}
}

void sendSCServer (lo_address *oscAddress) { //sets data
	char string[20];
	int i, j;
	int busNumber;
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 8; j++) {
			if (sensorData[i][j] != oscBuffer[i][j] && switchMatrix[i][j] == 1){
				busNumber = (i*10+j)+100;
				// printf("%i\n", busNumber);
				lo_send(*oscAddress, "/c_set", "if", busNumber, sensorData[i][j]);

				oscBuffer[i][j] = sensorData[i][j];
			}
		}
	}
}

void getSerial(int *fd) {
	char inbyte = 0;
	int index = 0;
	char mask1 = 240;
	char mask2 = 15;
	char digitalData[2];
	char mux;
	char sensor;

	while (serialDataAvail (*fd)) {
		inbyte = serialGetchar(*fd);
		// printf("%i\n", (int) inbyte); //print each byte in the serial stream

		if (index == 0 && inbyte == 253){
			index = 1;
		}
		else if (index == 1  && inbyte == 254) {
			index = 2;
		}
		else if (index >= 2 && index < 4 ) {
			digitalData[index-2] = inbyte;
			index = index + 1;
		} else if (index == 4 && inbyte == 255)
		{

			mux = (digitalData[0] & mask1) >> 4;
			sensor = (digitalData[0] & mask2);

			index = 0;

			sensorData[mux][sensor] = (digitalData[1]/250.0);

			//print changed values
			// printf("%i\n", (int) mux);
			// printf("%i\n", (int) sensor);
			// printf("%i\n", (int) digitalData[1]/250.0);

		} else {
			index = 0;
		};

	}

	serialFlush(*fd);
}

void printSensorArray (){
	int i, j;
	system("clear");
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 8; j++) {
			printf("%f", sensorData[i][j]);
			printf("\\");
		}
		printf("\n");
	}

}

void getHostname(){
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	// printf("%s\n", hostname);
};
