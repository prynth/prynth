#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <lo/lo.h>


int main ()
{
	int fd ;
	int count ;

	if ((fd = serialOpen ("/dev/ttyAMA0", 9600)) < 0)
	{
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
		return 1 ;
	}

	if (wiringPiSetup () == -1)
	{
		fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
		return 1 ;
	}

	while (TRUE){	

		char digitalData[2];
		char sensorData[10];
		char inbyte = 0;
		int index = 0;

		while (serialDataAvail (fd)) {
			inbyte = serialGetchar(fd);
		
			// printf("%i\n", (int) inbyte);

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
				sensorData[digitalData[0]] = digitalData[1];
				index = 0;
			} else {
				index = 0;
			};

		}
		// printf("%i / ", (int) sensorData[0]);
		// printf("\n");
		fflush (stdout);
		//serialFlush(fd);
		delay (1) ;
	}



	return 0 ;
}
