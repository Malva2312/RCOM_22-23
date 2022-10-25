// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"

#include <stdio.h>
#include <string.h>

#define READ_MODE 'r'
#define WRITE_MODE 'w'

#define MAX_BYTES 500

enum ControFiedl{
    NONE,
    DATA, START, END
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



    if (layer.role == LlTx){
        sleep(2);
        unsigned char buff[50];
        strcpy(buff, "\nHELLO WORLD!\0");
        llwrite(buff, sizeof(buff));
    }

    else if (layer.role == LlRx){
        printf("HERE");
        unsigned char *packet;
        while (llread(&packet) >0)
        {
            printf("HERE2");
        }

        //llread(packet);
    }

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
    llclose(0);
}
