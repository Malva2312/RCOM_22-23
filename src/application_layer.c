//Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

#define READ_MODE 'r'
#define WRITE_MODE 'w'

#define MAX_BYTES 500

enum ControFiedl{
    NONE, DATA, START, END
};

int N = 0;
void increaseN(){
    if (++N>=255) N = 0;
}


void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    printf("\nStart Protocol");
    LinkLayer layer;
    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;
    if (strcmp(role, "tx") == 0)
        layer.role = LlTx;
    if (strcmp(role, "rx") == 0)
        layer.role = LlRx;
    strcpy(layer.serialPort, serialPort);
    layer.timeout = timeout;

    if (llopen(layer) < 0)
    {
        printf("\nHere");
        perror("\nCould not link\n");
        exit(-1);
    }

    printf("\nConnected through serial port");


    clock_t start, end;
    
    start = clock();

    if (layer.role == LlTx){
        sleep(2);
        unsigned char packet[300], bytes[200];
        int nBytes = 200, curByte=0, index=0, nSequence = 0;

        FILE *fileptr;      

        fileptr = fopen(filename, "rb");
        if(fileptr == NULL){
            printf("File not found\n");
            return;
        }
        
        int packetSize = getControlPacket(filename,1,&packet);
        llwrite(packet, packetSize);
        
        int fileNotOver = 1;

        while(fileNotOver){

            //EOF
            if(!fread(&curByte, (size_t)1, (size_t) 1, fileptr)){
                fileNotOver = 0;
                packetSize = getDataPacket(bytes, &packet, nSequence++, index);

                if(llwrite(packet, packetSize) == -1){
                    return;
                }
            }

            //index = 200, all the Bytes
            else if(nBytes == index) {
                packetSize = getDataPacket(bytes, &packet, nSequence++, index);

                if(llwrite(packet, packetSize) == -1){
                    return;
                }

                memset(bytes,0,sizeof(bytes));
                memset(packet,0,sizeof(packet));
                index = 0;
            }

            bytes[index++] = curByte;
        }

        fclose(fileptr);
        packetSize = getControlPacket(filename,0,&packet);

        if(llwrite(packet, packetSize) == -1){
            return;
        }
    }

    else if (layer.role == LlRx){
        
        FILE *fileptr;
        char readBytes = 1;
        
        
        while(readBytes){
        
            unsigned char packet[600] = {0};
            int sizeOfPacket = 0, index = 0;
            
            if(llread(&packet, &sizeOfPacket)==-1){
                continue;
            }
           
            
            if(packet[0] == 0x03){
                printf("\nClosed penguin\n");
                fclose(fileptr);
                readBytes = 0;
            }
            else if(packet[0]==0x02){
                printf("\nOpened penguin\n");
                fileptr = fopen(filename, "wb");   
            }
            else{
                for(int i=4; i<sizeOfPacket; i++){
                    fputc(packet[i], fileptr);
                }
            }
        }
    }

    end = clock();
    float duration = ((float)end - start)/CLOCKS_PER_SEC; 

    llclose(0);

    return;

    /*
    if (layer.role == LlTx)
    {   
       // N = START;
        int file = open(filename, READ_MODE);
        if (file < 0)
        {
            perror("\nERROR OPENING FILE");
            exit(-1);
        }
        printf("\nFILE OPENED");
        int buf_size = MAX_BYTES;
        unsigned char buffer[buf_size];
        // int write_result = 0;
        int bytes;

        do
        {
            bytes = read(file, buffer, buf_size);
            if (bytes < 0)
            {
                printf("\nERRO READING FILE");
                break;
            }
            else if (bytes == 0) // stop
            {
                buffer[0] = '\0';
                llwrite(buffer, 1);
                printf("\n\nDone reading file\n");
                break;
            }
            else if (bytes > 0)
            {
                if (llwrite(buffer, buf_size) < 0)
                {
                    perror("\nERROR SEND DATA TO LINK LAYER");
                    exit(-1);
                }
            }
        } while ((bytes) > 0);
    }
    else if (layer.role == LlRx){
        int file = open(filename, WRITE_MODE);
        if (file < 0)
        {
            perror("\nERROR OPENING FILE");
            exit(-1);
        }
        unsigned char *packet;
        llread(packet);

        while (strcmp(packet, '\0')){
            llread(packet);
        }

    }
    */
}

int getControlPacket(char* filename, int start, unsigned char* packet){

    int sizeOfPacket = 0;

	if(strlen(filename) > 255){
        printf("size of filename couldn't fit in one byte: %d\n",2);
        return -1;
    }

	unsigned char hex_string[20];

    struct stat file;
    stat(filename, &file);
    sprintf(hex_string, "%02lX", file.st_size);
	
	int index = 3, fileSizeBytes = strlen(hex_string) / 2, fileSize = file.st_size;

    printf("\nfilesize: %d\n hex_string: %s", file.st_size, hex_string);

    if(fileSizeBytes > 256){
        printf("size of file couldn't fit in one byte\n");
        return -1;
    }
    
    if(start){
		packet[0] = 0x02; 
    }
    else{
        packet[0] = 0x03;
    }

    packet[1] = 0x00; // 0 = tamanho do ficheiro 
    packet[2] = fileSizeBytes;


	for(int i=(fileSizeBytes-1); i>-1; i--){
		packet[index++] = fileSize >> (8*i);
	}
    

    packet[index++] = 0x01;
    packet[index++] = strlen(filename);

	for(int i=0; i<strlen(filename); i++){
		packet[index++] = filename[i];
	}

	sizeOfPacket = index;
    
    return sizeOfPacket;

}

int getDataPacket(unsigned char* bytes, unsigned char* packet, int nSequence, int nBytes){

	int l2 = div(nBytes, 256).quot , l1 = div(nBytes, 256).rem;

    packet[0] = 0x01;
	packet[1] = div(nSequence, 255).rem;
    packet[2] = l2;
    packet[3] = l1;

    for(int i=0; i<nBytes; i++){
        packet[i+4] = bytes[i];
    }

	return (nBytes+4); //tamanho do data packet

}