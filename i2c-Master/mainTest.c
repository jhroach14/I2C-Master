#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define I2C_BUS 0x2A

static const char *I2C_ADAPTER = "/dev/i2c-1";

int startI2C();
void setSlave(int i2cBus, unsigned int slvAddr);
void printDevices();
void writeCommand(int i2cBus, unsigned int cmdCode, unsigned int param);
void readResponse(int i2cBus, char *response);


int main() {
	
	int i2cBus = startI2C();
	int flag =1;
	
	while(flag){
		
		unsigned int slvAddr = 0;
		int flag2 = 1;
	
		printDevices();
		scanf("%x", &slvAddr);
		setSlave(i2cBus, slvAddr);
		
		while(flag2){
			
			unsigned int commandCode = 0;
			unsigned int parmeterByte = 0;
			unsigned int wait = 0;
			char rspByts[2];
		
			printf("I2C-TEST: What command code byte do you want to send (hex)? 0x");
			scanf("%x",  &commandCode);
			printf("I2C-TEST: What parameter byte do you want to send (hex)? 0x");
			scanf("%x",  &parmeterByte);
			printf("I2C-TEST: What should the read delay be (ms)? ");
			scanf("%u",  &wait);
			
			writeCommand(i2cBus, commandCode, parmeterByte);
			usleep(wait);
			
			readResponse(i2cBus, rspByts);
			printf("I2C-TEST: Response packet = 0x%x%x", rspByts[1], rspByts[2]);
		}//while2
	}//while1
	return(EXIT_SUCCESS);
}

void readResponse(int i2cBus, char *response){
	
	char buf[2];
	
	printf("I2C-TEST: Reading Response... ");
	if(read(i2cBus, buf, 2) != 2){
		printf("Failed to read response.\n");
		exit(0);
	}
	
	strncpy(response, buf, 2);
	
}

void writeCommand(int i2cBus, unsigned int cmdCode, unsigned int param){
	
	char buf[2];
	buf[0] = cmdCode;
	buf[1] = param;
	
	printf("I2C-TEST: Sending command 0x%x with parameter 0x%x... ", cmdCode, param);
	if(write(i2cBus, buf, 2) != 2){
		printf("Failed to send command.\n");
		exit(0);
	}
	printf("Success.\n");
	
}

//opens adapter stream for R/W
int startI2C() {
  //attempting to open the I2C bus as a R/W file
  printf("I2C-TEST: Initializing I2C interface... ");
  int fd;
  if ( (fd = open(I2C_ADAPTER, O_RDWR)) < 0 ) {
	printf("Failed to access device %s\n", I2C_ADAPTER);
    exit(1);
  }
  
  printf("Success\n");
  return fd;

}

//sets recipient of commands on bus
void setSlave(int i2cBus,unsigned int slvAddr){
	 
	printf("I2C-TEST: Acquiring bus to slave node at 0x%x... ", slvAddr);
	
	if ( ioctl(I2C_BUS, I2C_SLAVE, slvAddr) < 0 ) {
		printf("Failed to acquire bus access\n");
		exit(1);
	} else {
		printf("Success\n");
	} //if-else
} 
	 
void printDevices(){
	
	char * devList = "I2C-TEST: Which device would you like to command (hex)?\n"
							 "\tEPS_BATTERY - 0x2A\n"
							 "\tEPS_MTHBD - 0x2A\n"
							 "I2C-TEST: 0x";
	
	printf(devList);
	
}
