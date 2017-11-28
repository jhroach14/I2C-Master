// v0.0.1 completed 7/3/17 by James Roach. Adam king is cited for providing a good bit of the i2c master code
//The purpose of this program is to connect to a slave device over the I2c protocol, send that device commands,
//and evaluate the response received for testing purposes.
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define BATTERY_ADDR 0x2A
#define EPS_MTHBD_ADDR 0x2B
#define UHF_ADDR 0x25

#define DATA_FILE "masterTest.data"//data file for auto mode

#define GREEN "\x1B[32m"
#define RED "\x1B[31m"
#define RESET "\x1B[0m"

static const char *I2C_ADAPTER = "/dev/i2c-1";
int i2cBus;//file descriptor 

//holds auto input data
struct cmdData{
	char cmdBytes[2];
	char ansBytes[2];
};

int startI2C();//connect as master node to bus
void setReceiverSlave( unsigned int slvAddr);//pass in address of slave you wish to message
void printDevices();
//void writeCommand(unsigned int cmdCode, unsigned int param);//write to bus
void writeCommand(unsigned int buff[], int n);
//void readResponse(char *response); //fills char* with response bytes
void readResponse(unsigned int buff[], int n);
int getDataFile(char *dataBuf);
int getDataSize();
int getNumCommands(char * dataBuf, int dataLen);//parse helper

int main(int argc, char* argv[]) {
	
	char* mode = "none";//mode sets whether it gets input from the user or from test files
	if(strcmp(argv[1], "-i")==0){
		mode = "interactive";
	}else
	if(strcmp(argv[1], "-a")==0){
		mode = "automatic";
	}else{
		printf("I2C-TEST: Error invalid flag %s. Usage -i or -a for interactive/auto. Exiting...", argv[1]);
		exit(0);
	}

	i2cBus = startI2C(); //connect to bus
	
	if(strcmp(mode, "interactive")==0){
	
		int flag =1;
		while(flag){
		
			unsigned int slvAddr = 0;
			int flag2 = 1;
	
			printDevices();//get device to talk to
			scanf("%x", &slvAddr);
			setReceiverSlave(slvAddr);//after this every packet sent will lead with an address byte of the recipient
						
			//char rspBytes[2];
			while(flag2){
			        	
				//unsigned int commandCode = 0;
				//unsigned int parmeterByte = 0;
				int numBytes = 0;	
				char wr = 0;
				
				do{
					printf("I2C-TEST: r or w + numBytes: ");
					scanf("%c %d", &wr, &numBytes);
				}while((wr != 'w' || wr != 'r') && (numBytes < 1));				

				unsigned int buff[numBytes];

				if(wr == 'w'){
					
					for(int i=0; i<numBytes; i++){
						printf("I2C-TEST: Byte %d = 0x",i+1);
						scanf("%x", &(buff[i]));
					}
					
					writeCommand(buff, numBytes);
					
				}else
				if(wr == 'r'){
					
					readResponse(buff, numBytes);
					printf("I2C-TEST: Response:");
					for(int i=0; i<numBytes; i++){
						printf(" 0x%02hhx", buff[i]);
					}
					/*readResponse(buff, numBytes);
					printf("I2C-TEST: Response:");
					for(int i=0; i<numBytes; i++){
						printf(" 0x%02hhx", buff[i]);
					}*/
				}
				puts("\n");
				
				printf("I2C-TEST:-----\n");	
				sleep(1);							

			}//while2
		}//while1
	}else//interactive
	if(strcmp(mode, "automatic")==0){
		
		char* dataBuff = malloc(getDataSize()+1);
		int dataLen = getDataFile(dataBuff);//read input file
		dataBuff[dataLen] = '\0';
		int cmdNum = getNumCommands(dataBuff, dataLen);
		struct cmdData commands[cmdNum];

		char address[2];
		address[0] = dataBuff[0];//chars at index 0 and 1 in input file are slave addr
		address[1] = dataBuff[1];
		
		unsigned int slvAddr = (unsigned int) strtol(address,NULL, 16);//fun times converting string hex to actual hex
		setReceiverSlave(slvAddr);
		printf("I2C-TEST: Parsing input data...\n");
		for(int i =0, cmdCount = 0; cmdCount<cmdNum; i++){
			if(dataBuff[i] == ':'){

				char cmd[2];
				char prm[2];
				char ans1[2];
				char ans2[2];
				
				cmd[0] = dataBuff[i-4];
				cmd[1] = dataBuff[i-3];
				prm[0] = dataBuff[i-2];
				prm[1] = dataBuff[i-1];	
		
				ans1[0] = dataBuff[i+1];
				ans1[1] = dataBuff[i+2];//all of this pulls the data out of the buffer
				ans2[0] = dataBuff[i+3];//based on its index in relation to the : char
				ans2[1] = dataBuff[i+4];

				struct cmdData *curCmd = &commands[cmdCount];
				curCmd->cmdBytes[0] = (char) strtol(cmd, NULL, 16);
				curCmd->cmdBytes[1] = (char) strtol(prm, NULL, 16);//fill cmd array with cmd data
				curCmd->ansBytes[0] = (char) strtol(ans1, NULL, 16);//including expected answer
				curCmd->ansBytes[1] = (char) strtol(ans2, NULL, 16);	
								
				cmdCount++;
				printf("\t0x%02X0x%02X:0x%02X0x%02X\n",curCmd->cmdBytes[0],curCmd->cmdBytes[1], curCmd->ansBytes[0], curCmd->ansBytes[1]);
			}
		}
		
		char rspBytes[2];
		for(int i = 0; i<cmdNum; i++){//execute commands
			sleep(1);
			//writeCommand(commands[i].cmdBytes[0], commands[i].cmdBytes[1]);			
			sleep(1);
			//readResponse(rspBytes);

			if(rspBytes[0] == commands[i].ansBytes[0] && rspBytes[1] == commands[i].ansBytes[1]){//success
				printf("I2C-TEST: Test for 0x%02X, 0x%02X "GREEN"PASSED"RESET" with response 0x%02X, 0x%02X\n",commands[i].cmdBytes[0], commands[i].cmdBytes[1], rspBytes[0], rspBytes[1]);	
			}else{//failure
				printf("I2C-TEST: Test for 0x%02X, 0x%02X "RED"FAILED"RESET" with response 0x%02X, 0x%02X\n",commands[i].cmdBytes[0], commands[i].cmdBytes[1], rspBytes[0], rspBytes[1]);	
			}
		}	

	}
	return(EXIT_SUCCESS);
}

int getNumCommands(char* dataBuff, int dataLen){

	int count =0;
	for(int i=0; i<	dataLen; i++){
		if(dataBuff[i]==':'){
			count++;
		}	
	}
	printf("I2C-TEST: %d commands detected\n", count);

	return count;
}

int getDataFile(char * dataBuff){
	
	printf("I2C-TEST: Reading %s... \n",DATA_FILE);

	int fd;
	int fileSize;	

	if((fd=open(DATA_FILE, O_RDONLY))==-1){
		perror("file open\n");
	}

	fileSize = lseek(fd, 0, SEEK_END);
	printf("I2C-TEST: File size = %d\n", fileSize);
	lseek(fd,0,SEEK_SET);
		
	int bytesRead;
	if((bytesRead = read(fd, dataBuff, fileSize))<0){
		perror("file read \n");
	}
	printf("I2C-TEST: %d bytes read\n", bytesRead);

	if(close(fd)<0){
		perror("file close\n");
	}
	
	return fileSize; 

}

int getDataSize(){

	int fd;
	int fileSize;
	
	if((fd=open(DATA_FILE, O_RDONLY))==-1){
		perror("file open\n");
	}

	fileSize = lseek(fd, 0, SEEK_END);
	printf("I2C-TEST: File size = %d\n", fileSize);
	lseek(fd,0,SEEK_SET);
	
	if(close(fd)<0){
		perror("file close2\n");
	}
	
	return fileSize;	
}

/*void readResponse(char * rspBytes){

	char buf[2];
	
	if(read(i2cBus, buf, 2) != 2){
		printf("Failed to read response\n");
		exit(0);
	}
	
	strncpy(rspBytes, buf, 2);

}*/

void readResponse(unsigned int buff[], int n){
	int count =0;
	unsigned int buf[n];
	if((count = read(i2cBus, buf, 4*n)) < 0){
		perror("READ");
		exit(0);
	}

	/*for(int i=0;i<n;i++){
		printf(" buff %d  = %02hhx ",i,buf[i]);
	}*/
	strncpy(buff, buf, 4*n);

	printf("I2C-TEST: Attempted to read %d bytes. %d bytes successfully read.\n", n, count);
}

void writeCommand(unsigned int buff[], int n){

	printf("I2C-TEST: Sending %d bytes... ", n);
	int count =0;
	if((count = write(i2cBus, buff, 4*n)) < 0){
		perror("WRITE");
		exit(0);
	}

	printf(" %d bytes sent.\n", count);

} 

/*void writeCommand(unsigned int cmdCode, unsigned int param){
	
	char buf[2];
	buf[0] = cmdCode;
	buf[1] = param;
	
	printf("I2C-TEST: Sending 0x%x 0x%x... ", cmdCode, param);
	if(write(i2cBus, buf, 2) != 2){
		printf("Failed to send command.\n");
		exit(0);
	}
	printf("Success.\n");
	
}*/

//opens adapter stream for R/W
int startI2C() {
  //attempting to open the I2C bus as a R/W file
  printf("I2C-TEST: Initializing I2C connection %s ... ",I2C_ADAPTER);
  int fd;
  if ( (fd = open(I2C_ADAPTER, O_RDWR)) < 0 ) {
	printf("Failed to access device %s\n", I2C_ADAPTER);
    exit(1);
  }
  
  printf("Success\n");
  return fd;

}

//sets recipient of commands on bus
void setReceiverSlave(unsigned int slvAddr){
	 
	printf("I2C-TEST: Acquiring slave node 0x%x... ", slvAddr);
	
	if ( ioctl(i2cBus , I2C_SLAVE, slvAddr) < 0 ) {
		printf("Failed to acquire bus access\n");
		exit(1);
	} else {
		printf("Success\n");
	} //if-else
} 
	 
void printDevices(){
	char args[5] = "-y 1";
	//args[4] = NULL;
	
	if(fork()==0){
		execvp("i2cdetect",args);
	}else{
		wait();
	}
	char * devList = "I2C-TEST: Pick a device (hex)?\n"
							 "\tEPS_BATTERY - 0x2A\n"
							 "\tEPS_MTHBD - 0x2B\n"
							 "\tUHF_TRANS - 0x25\n"
							 "I2C-TEST: 0x";
	
	printf(devList);
	
}
