// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PACKET_SIZE 256
#define MAX_BUFFER_SIZE (MAX_PACKET_SIZE * 2 + 7)

// MISC
#define FALSE 0
#define TRUE 1

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

// SET buffer values
#define FLAG 0x7E
#define A_T 0x03
#define A_R 0x01
#define C_SET 0x03
#define C_UA 0x07
#define BCC(a, c) (a^c) 
#define DISC 0x0B
#define RR(n) 0x05 | (n << 7) // 0 ou 1
#define REJ(n) 0x01 | (n << 7) // 0 ou 1
#define C_INF(n) 0x00 |(n<<6) // 0 ou 1

#define RR_R 0

#define ESCAPE 0x7D

#define START 0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC_OK 4
#define S_STOP 5
#define WAITING_DATA 6

// Responses
#define RR0 0
#define RR1 1
#define REJ0 2
#define REJ1 3
#define OTHER 4

//Packets
#define MIDDLE_PACKET 1
#define STARTING_PACKET 2
#define ENDING_PACKET 3

#define FILE_SIZE 0
#define FILE_NAME 1

// Returns role
LinkLayerRole getRole();

// Returns number of transmissions
int getnTransmissions();

// Returns timeout
int getTimeOut();

// State machine
int stateMachine(unsigned char a, unsigned char c, int isData, int RR_REJ);

// Fills tram
int sendBuffer(unsigned char a, unsigned char c);

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char * buffer);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

#endif // _LINK_LAYER_H_
