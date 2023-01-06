// Application layer protocol implementation

#include "application_layer.h"

struct stat file_stat;

clock_t start_t, end_t;
double total_t;


int transmitter(const char *filename)
{
    start_t = clock();

    if (stat(filename, &file_stat) < 0)
    {
        perror("Error getting file information.");
        return -1;
    }

    FILE *fptr = fopen(filename, "rb");

    // Starting packet
    unsigned L1 = sizeof(file_stat.st_size);
    unsigned L2 = strlen(filename);
    unsigned packet_size = 5 + L1 + L2;

    unsigned char packet[packet_size];
    packet[0] = STARTING_PACKET;
    packet[1] = FILE_SIZE;
    packet[2] = L1;
    memcpy(&packet[3], &file_stat.st_size, L1);
    packet[3 + L1] = FILE_NAME;
    packet[4 + L1] = L2;
    memcpy(&packet[5 + L1], filename, L2);

    if (llwrite(packet, packet_size) < 0)
        return -1;

    printf("Starting packet sent\n");

    // Middle packets
    unsigned char buf[MAX_PACKET_SIZE];
    unsigned bytes_to_send;
    unsigned sequenceNumber = 0;

    while ((bytes_to_send = fread(buf, sizeof(unsigned char), MAX_PACKET_SIZE - 4, fptr)) > 0)
    {
        printf("MIDLE PACKET\n");
        unsigned char dataPacket[MAX_PACKET_SIZE];
        dataPacket[0] = MIDDLE_PACKET;
        dataPacket[1] = sequenceNumber % 255;
        dataPacket[2] = (bytes_to_send / 256);
        dataPacket[3] = (bytes_to_send % 256);
        memcpy(&dataPacket[4], buf, bytes_to_send);

        llwrite(dataPacket, ((bytes_to_send + 4) < MAX_PACKET_SIZE) ? (bytes_to_send + 4) : MAX_PACKET_SIZE);
        printf("Sent %dº data package\n", sequenceNumber);
        sequenceNumber++;
    }
    
    printf("Midle packets sent\n");

    // Ending packet
    L1 = sizeof(file_stat.st_size);
    L2 = strlen(filename);
    packet_size = 5 + L1 + L2;

    packet[packet_size];
    packet[0] = ENDING_PACKET;
    packet[1] = FILE_SIZE;
    packet[2] = L1;
    memcpy(&packet[3], &file_stat.st_size, L1);
    packet[3 + L1] = FILE_NAME;
    packet[4 + L1] = L2;
    memcpy(&packet[5 + L1], filename, L2);

    if (llwrite(packet, packet_size) < 0)
        return -1;

    printf("Ending packet sent\n");

    fclose(fptr);

    end_t = clock();
    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;

    printf("\nTotal time taken: %f seconds\n", total_t);
    printf("Size transfered: %d bytes\n", file_stat.st_size);
    printf("Transfer Speed: %f B/s\n\n", file_stat.st_size/total_t);


    return 0;
}

int receiver(const char *filename)
{
    int s;
    int nump = 0;
    int numTries = 0;
    static FILE *destination;
    int stop = FALSE;
    while (stop == FALSE)
    {
        printf("\nRECEIVER\n");

        unsigned char buf[MAX_PACKET_SIZE];
        if ((s = llread(buf)) < 0)
            continue;
        printf("PACKET : %d\n", buf[0]);
        switch (buf[0])
        {
        case STARTING_PACKET:
            destination = fopen(filename, "wb");
            break;

        case MIDDLE_PACKET: ;
            static unsigned int n = 0;
            if (buf[1] != n)
                return -1;
            n = (n + 1) % 255;

            unsigned int data_size = buf[2] * 256 + buf[3];
            fwrite(&buf[4], sizeof(unsigned char), data_size * sizeof(unsigned char), destination);

            break;

        case ENDING_PACKET:
            printf("\nENDING PACKET\n");
            close(destination);
            stop = TRUE;
            break;
        }
    }
    return 0;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer layer;

    layer.baudRate = baudRate;
    layer.nRetransmissions = nTries;

    if (strcmp(role, "tx") == 0)
        layer.role = LlTx;
    if (strcmp(role, "rx") == 0)
        layer.role = LlRx;

    sprintf(layer.serialPort, "%s", serialPort);
    layer.timeout = timeout;

    if (llopen(layer) < 0)
        return -1;

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
