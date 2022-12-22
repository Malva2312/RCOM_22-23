// Application layer protocol implementation

#include "application_layer.h"

int transmitter(const char *filename)
{
    int f;
    struct stat file_stat;

    if (stat(filename, &file_stat) < 0)
    {
        perror("Error getting file information.");
        return -1;
    }

    if (f = open(filename, O_RDONLY) < 0)
    {
        perror("Error opening file.");
        return -1;
    }

    unsigned char example[5];

    example[0] = 0x7e;
    example[1] = 0X01;
    example[2] = 0X02;
    example[3] = 0x03;
    example[4] = ESCAPE;
    
    llwrite(example, 5);

    return 0;
}

int receiver(const char *filename)
{
    unsigned char buf[MAX_BUFFER_SIZE];

    llread(buf);

    return 0;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;

    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;

    if (strcmp(role, "tx") == 0) layer.role = LlTx;
    if (strcmp(role, "rx") == 0) layer.role = LlRx;

    sprintf(layer.serialPort, "%s", serialPort);
    layer.timeout = timeout;

    llopen(layer);

    switch (layer.role)
    {
    case LlTx:
        transmitter(filename);
        break;
    case LlRx:
        receiver(filename);
        break;
    default:
        break;
    }

    llclose(FALSE);

}