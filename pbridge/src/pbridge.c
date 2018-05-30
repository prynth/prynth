#include <stdio.h>  //printing
#include <string.h> //string manipulation for osc
#include <errno.h> //error catching
#include <time.h> //clocks
#include <math.h> //rounding
#include <wiringSerial.h>
#include <lo/lo.h>
#include <stdlib.h>
#include <stdbool.h>

//global variable initialization
int fd ; //serial
char hostname[1024];

float sensorData[16][8] = {0};
float oscBuffer[16][8] = {0};
char sensorName[16][8][50];

char byteData[80];

// For data reception
int inCount = 0;
int esc = 0;
char message[20];
int dataReady = 0;
bool serialBusy = false;

//method declaration
void getSerial ();
void getSerial2 (int *fd);
void printSensorArray ();
void sendOSC();
void getHostname();
void sendSerial (char* packet, int len);
void parseData(char* digitalData, int len);
void setSensorFilter(char mux, char sensor,
					char filterType, float cutOff,
					float beta, float diffFiltCutOff);
void setSampleRate(float sampRate);
void setSensorActive(char mux, char sensor, char mask);
void setSensorResolution(char mux, char sensor, char resolution);
void float2byte(char b[], float f);
int sendAndWaitForAck(char* data, int len);
void sendSystemMessage(char* message);

//methods for osc reception
void error(int num, const char *m, const char *path);
int sensorFilter_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sampleRate_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sensorActive_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sensorResolution_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sensorName_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sensorReset_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);
int sensorSelected_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data);

int mode; // 1 send to server, 2 send to lang
int debug = 0; // for printing debug messages

//set oscAddress of supercollider language and server
lo_address scLangAddress; //socket address scLang
lo_address scServerAddress; //socket address scServer
lo_address serverAddress; //osc address

// for monitoring sensor values on the browser
int monitorMux = -1;
int monitorSensor = -1;

#define BILLION 1000000000L

int main (int argc, char *argv[])
{
	int i,j;
	char string[20];
	for (i=0; i<16; i++)
	{
		for (j=0; j<8; j++)
		{
			sprintf(string, "/%i/%i", i, j);
			strcpy(sensorName[i][j], string);
		}
	}

	if( argc == 2 )
	{
		// printf("The argument supplied is %s\n", argv[1]);
		if ( strcmp(argv[1], "-s") == 0 )
		{
			mode = 1;
		}
		if ( strcmp(argv[1], "-l") == 0 )
		{
			mode = 2;
		}
	}
	else if( argc == 3 )
	{
		if ( strcmp(argv[1], "-s") == 0 )
		{
			mode = 1;
		}
		if ( strcmp(argv[1], "-l") == 0 )
		{
			mode = 2;
		}
		if ( strcmp(argv[2], "-d") == 0 )
		{
			debug = 1;
			printf("OSC debug enabled\n");
		}
	}
	else
	{
		printf("One or two arguments expected. 1st: -s for server -l for lang, 2nd: -d for debug.\n");
	}

	// variable setup

	struct timespec timeStart, timeEnd; //clock types
	uint64_t timeDiff = 0; //time differential
	uint64_t timeTotal = 0; //in ms
	uint64_t serialTimeStamp = 0;
	uint64_t oscTimeStamp = 0;
	uint64_t clearTimeStamp = 0;
	uint64_t printTimeStamp = 0;
	uint64_t showTimeDiff;

	int sendCount = 0;

	// start a new server on port 57100
    lo_server_thread rc = lo_server_thread_new("57100", error);

	// add method for filter parameter changing
	lo_server_thread_add_method(rc, "/sensorFilter", "sifff", sensorFilter_handler, NULL);
	lo_server_thread_add_method(rc, "/sampleRate", "f", sampleRate_handler, NULL);
	lo_server_thread_add_method(rc, "/sensorActive", "si", sensorActive_handler, NULL);
	lo_server_thread_add_method(rc, "/sensorResolution", "si", sensorResolution_handler, NULL);
	lo_server_thread_add_method(rc, "/sensorName", "ss", sensorName_handler, NULL);
	lo_server_thread_add_method(rc, "/sensorReset", "i", sensorReset_handler, NULL);
	lo_server_thread_add_method(rc, "/sensorSelected", "ii", sensorSelected_handler, NULL);

	//  might have to implement acknowledgement capability
	lo_server_thread_start(rc);

	scLangAddress = lo_address_new("127.0.0.1", "57120");
	scServerAddress = lo_address_new("127.0.0.1", "57110");
	serverAddress = lo_address_new("127.0.0.1", "57101");

	//open serial
	if ((fd = serialOpen ("/dev/ttyAMA0", 3000000 )) < 0)
	{
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}

	getHostname();

	serialClose(fd);

	if ((fd = serialOpen ("/dev/ttyAMA0", 3000000)) < 0)
	{
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}

	//serialFlush(fd);

	//lo_send(serverAddress, "/sendSensorConfig", "N");	//get sensor configuration values from the server.
	delay(50);

   	// main loop
	while (1)
	{
		clock_gettime(CLOCK_MONOTONIC, &timeStart); /* mark start time */

		delay (1); // has to spend some time

		if(serialBusy == false)	getSerial();
		if(dataReady == 1)
		{
			parseData(message, inCount);
			dataReady = 0;
		}

		if (timeTotal - oscTimeStamp >= 20)  //check if this affects system performance - this was probably there to avoid OSC channel overload
		{
			sendOSC();
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

void sendOSC()
{
	int i, j;
	for (i = 0; i < 16; i++)
	{
		for (j = 0; j < 8; j++)
		{
			if (sensorData[i][j] != oscBuffer[i][j])
			{
				if (mode == 2)
				{
					char oscAddress[1025];
					sprintf(oscAddress, "/%s%s", hostname, sensorName[i][j]);
					//printf("%s %f\n", oscAddress, sensorData[i][j]);
					lo_send(scLangAddress, oscAddress, "f", sensorData[i][j]);
				}
				else if (mode == 1)
				{
					int busNumber = (i*10+j)+100;
					//printf("%s %d %f\n", /c_set, busNumber, sensorData[i][j]);
					lo_send(scServerAddress, "/c_set", "if", busNumber, sensorData[i][j]);
				}
				if ((i == monitorMux) && (j == monitorSensor))
					lo_send(serverAddress, "/sensorMonitorValue", "f", sensorData[i][j]);
				oscBuffer[i][j] = sensorData[i][j];
			}
		}
	}
}

void getSerial()
{
	//uses global variables message and inCount.
	char inbyte;
	while (serialDataAvail (fd) != 0)
	{
		inbyte = serialGetchar(fd);
		if (inbyte == '\\')
		{
			if (esc == 1)
			{
				message[inCount++] = inbyte;
				esc = 0;
			}
			else
				esc = 1;
		}
		else if (esc == 1)
		{
			if (inbyte == '$')
			{
				inCount = 0;
				esc = 0;
				dataReady = 0;
			}
			else if(inbyte == '*')
			{
				dataReady = 1;
				esc = 0;
				return;
			}
		}
		else
		{
			message[inCount++] = inbyte;
		}
	}
}

void parseData(char* digitalData, int len)
{
	int i;
	//printf("%d\n",len);
	char mux;
	char sensor;
	char checkSum = 0;

	//printf("%s\n", digitalData);

	for (i = 0; i < len; i++)
		checkSum ^= digitalData[i];

	if (checkSum != 0)	//error in received data
	{
		//printf("Checksum error\n");
		return;
	}
	//printf("Checksum okay\t");
	mux = (digitalData[0]>>3) & 0x0F;
	sensor = digitalData[0] & 0x07;
	//printf("\t%d\t",mux);
	//printf("\t%d\n",sensor);

	if ((digitalData[0] & 0x80) == 0x80){
		//printf("\t%d\t",(digitalData[0] & 0x80)>>7);
		unsigned long l = 0;
		float l2f;
		l |= (((unsigned long)digitalData[1] << 24) & 0xFF000000);
		l |= (((unsigned long)digitalData[2] << 16) & 0x00FF0000);
		l |= (((unsigned long)digitalData[3] << 8) & 0x0000FF00);
		l |= (((unsigned long)digitalData[4]) & 0x000000FF);
		l2f = *(float*)&l;
		sensorData[mux][sensor] = l2f;
		//if (mux == 9 && sensor == 0)
		//printf("\t%f\n",sensorData[mux][sensor]);
		return;
	}
	else{
		//printf("\t%d\t",(digitalData[0] & 0x80)>>7);
		sensorData[mux][sensor] = digitalData[1] / 255.0;
		//if (mux == 0 && sensor == 0)
		//printf("\t%f\n", (float)digitalData[1]);
		return;
	}
}

void printSensorArray ()
{
	int i, j;
	system("clear");
	for (i = 0; i < 10; i++)
	{
		for (j = 0; j < 8; j++)
		{
			printf("%f", sensorData[i][j]);
			printf("\\");
		}
		printf("\n");
	}
}

void getHostname()
{
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	// printf("%s\n", hostname);
};


void error(int num, const char *msg, const char *path)
{
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
    fflush(stdout);
}

int sensorFilter_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
	char* oscPath = (char*)argv[0];
	char mux;
	char sensor;
	char filterType = (char)(argv[1]->i);
	float cutOff = argv[2]->f;
	float beta = argv[3]->f;
	float diffFiltCutOff = argv[4]->f;

	mux = (char)atoi(strtok(oscPath, "/"));
	sensor = (char)atoi(strtok(NULL, "/"));

	if (debug) printf("sensorFilter: %d\t%d\t%d\t%f\t%f\t%f\n", mux, sensor, filterType, cutOff, beta, diffFiltCutOff);
	setSensorFilter(mux, sensor, filterType, cutOff, beta, diffFiltCutOff);
}

//  make a separate function for setting sensor resolution on both serial2osc.c and in the teensy code
void setSensorFilter(char mux, char sensor, char filterType, float cutOff, float beta, float diffFiltCutOff)
{
	char digitalData[20];
	char b[4];
	int i;
	int response = 0;

	char packet[15] = {0};
	packet[0] = 0x10|filterType;
	packet[1] = (mux << 3) | (sensor);

	float2byte(b, cutOff)	;
	for(i = 0; i<4 ; i++)
		packet[2+i] = b[i];

	float2byte(b, beta);
	for(i = 0; i<4 ; i++)
		packet[6+i] = b[i];

	float2byte(b, diffFiltCutOff);
	for(i = 0; i<4 ; i++)
		packet[10+i] = b[i];

	for (i = 0; i < 14; i++)
		packet[14] ^= packet[i];

	response = sendAndWaitForAck(packet, 15);
}

int sampleRate_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
	float sampRate = argv[0]->f;
	if (debug) printf("sampleRate: %f\n", sampRate);
	setSampleRate(sampRate);
}

void setSampleRate(float sampRate)
{
	char packet[6] = {0};
	char b[4];
	char digitalData[20];
	int i;
	int response = 0;

	packet[0] = 0x00;	//message type
	float2byte(b, sampRate);
	for(i = 0; i<4; i++)
	{
		packet[i+1] = b[i];
		packet[5]^=packet[i+1];
	}

	response = sendAndWaitForAck(packet, 6);
}

int sensorActive_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
	char* oscPath = (char*)argv[0];
	char mux;
	char sensor;
	char active = (char)((argv[1]->i)%2);
	mux = (char)atoi(strtok(oscPath, "/"));
	sensor = (char)atoi(strtok(NULL, "/"));

	if (debug) printf("sensorActive: %d\t%d\t%d\n", mux, sensor, active);
	setSensorActive(mux, sensor, active);
}

void setSensorActive(char mux, char sensor, char active)
{
	char packet[3] = {0};
	int response = 0;

	packet[0] = 0x20;
	packet[1] = (active<<7) | (mux << 3) | (sensor);
	packet[2] = packet[0]^packet[1];

	response = sendAndWaitForAck(packet, 3);
}

int sensorResolution_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)
{
	char* oscPath = (char*)argv[0];
	char mux;
	char sensor;
	char resolution = (char)((argv[1]->i)%2);
	mux = (char)atoi(strtok(oscPath, "/"));
	sensor = (char)atoi(strtok(NULL, "/"));

	if (debug) printf("sensorResolution: %d\t%d\t%d\n", mux, sensor, resolution);
	setSensorResolution(mux, sensor, resolution);
}

void setSensorResolution(char mux, char sensor, char resolution)
{
	char packet[3] = {0};
	int response = 0;

	packet[0] = 0x30;
	packet[1] = (resolution<<7) | (mux << 3) | (sensor);
	packet[2] = packet[0]^packet[1];

	response = sendAndWaitForAck(packet, 3);
}

int sensorName_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)	//OSC alias for the sensor
{
	char* oscPath = (char*)argv[0];
	char mux;
	char sensor;
	mux = (char)atoi(strtok(oscPath, "/"));
	sensor = (char)atoi(strtok(NULL, "/"));

	strcpy(sensorName[mux][sensor],(char*)argv[1]);
	if (debug) printf("sensorName: %d\t%d\t%s\n", mux, sensor, sensorName[mux][sensor]);
}

int sensorReset_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data)	//Reset all sensor parameters on teensy
{
	int _reset = argv[0]->i;
	char packet[2] = {0};
	int response = 0;
	if (debug) printf("Reset all sensor parameters and deactivate all sensors\n");

	packet[0] = 0x40;
	packet[1] = ~packet[0];
	response = sendAndWaitForAck(packet, 2);
}

int sensorSelected_handler(const char *path, const char *types, lo_arg ** argv,
                    int argc, void *data, void *user_data) //send real-time values of sensor to browser
{
	int mux = argv[0]->i;
	int sensor = argv[1]->i;

	monitorMux = mux;
	monitorSensor = sensor;

	if (debug) printf("sensorSelected: %d\t%d\n", mux, sensor);
}

void sendSerial (char* packet, int len)
{
	int i;
	serialFlush(fd);
	serialPutchar(fd, '\\');
	serialPutchar(fd, '$');
	for (i = 0; i < len; i++)
	{
		if (packet[i] == '\\')
			serialPutchar(fd, '\\');
		serialPutchar(fd, packet[i]);
	}
	serialPutchar(fd, '\\');
	serialPutchar(fd, '*');
}

void float2byte(char b[], float f)
{
	unsigned long l = *((unsigned long*)&f);

	int i;
	for (i = 0; i < 4; i++)
	{
		b[i] = (l >> 8 * (3 - i)) & 0xFF;
	}
}

int sendAndWaitForAck(char* data, int len) //add timeout?
{
	struct timespec startTime, curTime;
	char ack = (char)data[1]^0x80;
	int numberOfAttempts = 0;
	serialBusy = true;
	while((ack != data[1]) && (numberOfAttempts <= 10))
	{
		sendSerial(data, len);
		clock_gettime(CLOCK_MONOTONIC, &startTime); /* mark start time */
		while (serialDataAvail (fd) == 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &curTime); /* mark current time */
			if((BILLION * (curTime.tv_sec - startTime.tv_sec) + curTime.tv_nsec - startTime.tv_nsec) > 500000000)
			{
				printf("%d - Response Timeout\n", numberOfAttempts);
				break;
			}
		} //wait for acknowledgement, quit loop if no response in 1 second.
		getSerial();
		ack = message[0];
		numberOfAttempts++;
		serialFlush(fd);
	}
	if((numberOfAttempts > 10) && (ack!=data[1]))
	{
		printf("Reached max. number of attempts \n");
		printf("Negative\n");
		sendSystemMessage("No response from Teensy. Reset Teensy to continue");
		serialFlush(fd);
		serialBusy = false;
		return 0;
	}
	else
	{
		if (debug) printf("Positive\n");
		serialFlush(fd);
		serialBusy = false;
		return 1;
	}
}

void sendSystemMessage(char* message)
{
	// lo_send(serverAddress, "/systemMessage", "s", message);
	lo_send(serverAddress, "/sensorMonitorValue", "s", message);
	if (debug) printf("System message: %s\n", message);
}
