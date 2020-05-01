#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <wiringPi.h>
#include <wiringSerial.h>

#define BAUDRATE 38400
void moveCursor(int fd, int loc){
	serialPutchar(fd, 0x1B);
	serialPutchar(fd, 0x48);
	serialPutchar(fd, loc);
}

void resetVFD(int fd){
	serialPutchar(fd, 0x1B); // ESC
	serialPutchar(fd, 0x49); // Initialize (pwr on state)
}

void clearVFD(int fd){
	serialPutchar(fd, 0x0E); // CLR
	serialPutchar(fd, 0x0C); // FF
	serialPutchar(fd, 0x0D); // CR
}

int flashColon(int fd, int state){
	moveCursor(fd, 0x04);
	if (state){
		serialPuts(fd, ":");
		state = 0;
	}
	else{
		serialPuts(fd, " ");
		state = 1;
	}

	return state;
}

int printTime(int fd, time_t now){
	/* Returns: 1-update date, 2-update weather */
	struct tm myTime = *localtime(&now);
	moveCursor(fd, 0x02);
	serialPrintf(fd, "%02d", myTime.tm_hour);
	moveCursor(fd, 0x05);
	serialPrintf(fd, "%02d", myTime.tm_min);
	if (myTime.tm_hour == 0 && myTime.tm_min == 0)
		return 1;
	else
		return 0;
}

void printDate(int fd, time_t now){
	struct tm myTime = *localtime(&now);
	char *DOW[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	char *month[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	moveCursor(fd, 0x08);
	serialPrintf(fd, DOW[myTime.tm_wday]);
	moveCursor(fd, 0x0C);
	serialPrintf(fd, month[myTime.tm_mon]);
	moveCursor(fd, 0x10);
	char buf[3];
	//TODO: Pad out date with spaces if only single digit
	sprintf(buf, "%d", myTime.tm_mday);
	serialPrintf(fd, buf);
}

char* readWeather(char *buf){
	/* Read weather file and store temp, weather in variables */

	FILE *f = fopen("weather.txt", "r");
	char tempBuf[10];
	char descBuf[25];

	fgets(tempBuf, 10, f);
	fgets(buf, 25, f);
	printf("%s\n", tempBuf);
	printf("%s\n", descBuf);

	fclose(f);
	return buf;
}

int main(char *argc, char *argv[]){
	int fd;
	int colonState;
	int counter = 0;
	char buffer[25] = {'\0'};

	fd = serialOpen("/dev/ttyAMA0", BAUDRATE);

	wiringPiSetup();
	
	//resetVFD(fd);
	clearVFD(fd);
	serialPutchar(fd, 0x16); // Hide cursor
	printDate(fd, time(0));
	printTime(fd, time(0));
	
	moveCursor(fd, 0x18);
	serialPutchar(fd, 0x95); // degree
	serialPuts(fd, "C");
	//serialPrintf(fd, " %s", readWeather(buffer));

	// Output weather to display
	moveCursor(fd, 0X1B);
	readWeather(buffer);
	for(int i=0; i<13; i++)
		serialPutchar(fd, buffer[i]);

	while(1){
		colonState = flashColon(fd, colonState);
		sleep(1);
		counter++;
		if (counter > 59){
			// Case statement for printTime return
			switch(printTime(fd,time(0))){
				case 1:
					printDate(fd, time(0));
					break;
				case 2:
					break;
			}
			//if(printTime(fd, time(0)) == 1)
			//	printDate(fd, time(0));
			counter = 0;
		}
	}

	serialClose(fd);
	return 0;
}
