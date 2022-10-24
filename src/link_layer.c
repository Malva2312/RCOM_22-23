// Link layer protocol implementation

#include "link_layer.h"

#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>

#include <string.h>
#include <signal.h>

#include <sys/stat.h>
#include <unistd.h>


// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define TRANSMISSOR 0
#define RECEIVER 1

#define SET_SIZE 5

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define C_UA 0x07
#define BCC_RCV A ^ C_SET

#define ESCAPE 0x7D

unsigned char BUFFER[SET_SIZE];

struct termios old_serial_port_settings;
struct termios new_serial_port_settings;

enum STATE
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP
} state = START;
// unsigned char C_received = 0x00;

int alarmEnabled = FALSE;
int alarmCount = 0;

LinkLayer new_layer;
int fd;

void setUpSerialPort(LinkLayer connectionParameters, int fd)
{
    // Save current port settings
    if (tcgetattr(fd, &old_serial_port_settings) < 0)
    {
        perror("\nFailed to get termios structure: tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&new_serial_port_settings, 0, sizeof(new_serial_port_settings));

    // setting baud rate
    new_serial_port_settings.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    new_serial_port_settings.c_iflag = IGNPAR;
    new_serial_port_settings.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    new_serial_port_settings.c_lflag = 0;
    new_serial_port_settings.c_cc[VTIME] = 0; // Inter-character timer unused
    new_serial_port_settings.c_cc[VMIN] = 0;  // Blocking read until 1 chars received

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &new_serial_port_settings) < 0)
    {
        perror("\nFailed to set serial attributes: tcsetattr");
        exit(-1);
    }
    printf("\nNew termios structure set\n");
}

void generateSetTrama(unsigned char C)
{
    // buffer = malloc(size * sizeof(unsigned char));

    BUFFER[0] = FLAG;
    BUFFER[1] = A;
    BUFFER[2] = C;
    BUFFER[3] = A ^ C;
    BUFFER[4] = FLAG;

    printf("\nTrama Set");
}

int stateMachine(int fd, unsigned char expected_C)
{
    //printf("\nStarting StateMachine");
    unsigned char byte;

    // maybe do not call return
    if (read(fd, &byte, 1) < 0)
    {
        perror("\nRead Failed on state machine");
        return -1;
    }

    switch (state)
    {
    case START: // FLAG
        if (byte == FLAG)
        {
            state = FLAG_RCV;
            //printf("\nFLAG_RCV");
        }
        else
        {
            state = START;
            //printf("\nStart");
        }
        break;

    case FLAG_RCV: // expecting A
        if (byte == A)
        {
            state = A_RCV;
            //printf("\nA_RCV");
        }
        else if (byte == FLAG)
        {
            state = FLAG_RCV;
            //printf("\nFLAG_RCV");
        }
        else
        {
            state = START;
            //printf("\nSTART");
            // otherwise e stays in the same state FLAG RCV4
        }

        break;

    case A_RCV:
        if (byte == expected_C)
        {
            state = C_RCV;
            //printf("\nC_RCV");
        }
        else if (byte == FLAG)
        {
            state = FLAG_RCV;
            //printf("FLAG_RCV");
        }
        else
        {
            state = START;
            //printf("\nSTART");
        }

        break;

    case C_RCV:
        if (byte == (A ^ expected_C))
        {
            state = BCC_OK;
            //printf("\nBCC_OK");
        }

        else
        {
            //printf("\nSTART");
            state = START;
        }

        break;

    case BCC_OK:
        if (byte == FLAG)
        {
            state = STOP;
            alarm(0);
            //printf("\nSTOP");
        }

        else
        {
            state = START;
            //printf("\nSTART");
        }

        break;

    default: 
        //printf("\nERRO ON STATEMACHINE");
        return -1;
        break;
    }
    //printf("\nEnded StateMachine");
    return 0;
    // return C_received;
}

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    //alarm(new_layer.timeout);
    //printf("\nALARM_ENABLED: %d\n", alarmEnabled);
    //printf("\nALARM_COUNTER: %d\n", alarmCount);
    return;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    (void)signal(SIGALRM, alarmHandler);
    new_layer = connectionParameters;
    // unsigned char *SET = NULL;

    const char *serialPortName = new_layer.serialPort;

    fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    setUpSerialPort(new_layer, fd);
    // printf("\n%d", connectionParameters.role);

    int stop = FALSE;

    switch (new_layer.role)
    {
    case LlTx:; // TRANSMISSOR:

        while (stop == FALSE && alarmCount < new_layer.nRetransmissions)
        {
            //printf("\nLOOP START: %d" , alarmEnabled);
            if (alarmEnabled == FALSE)
            {
                //printf("\n[LOG] Sending SET #%d\n", alarmCount);
                //printf("\nHERE: %d", alarmEnabled);
                //printf("\nalarm nº:%d", alarmCount);
                generateSetTrama(C_SET);
                int buffer_size = sizeof(BUFFER);
                if (write(fd, BUFFER, buffer_size) > 0)
                {
                    printf("\nC_SET enviado\n");
                }

                // Now that we sent SET, let's call the alarm and read write
                alarm(new_layer.timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
                state = START;
            }

            stateMachine(fd, C_UA);

            if (state == STOP)
            {
                stop = TRUE;
                printf("\nTrama Received\n");
                
            }
            // state = START;
            // stop = TRUE;
        }
        state = START;
        //printf("\nSTART");
        if (alarmCount >= new_layer.nRetransmissions){
            printf("\nTime Out\n");
        }
        alarm(0);

        break;

    case LlRx:; // RECEIVER:
        while (state != STOP)
        {
            stateMachine(fd, C_SET);
        }
        state = START;
        stop = TRUE;

        generateSetTrama(C_UA);
        if (write(fd, BUFFER, SET_SIZE) > 0)
        {
            printf("\nC_UA enviado\n");
        }

    default:
        break;
    }
    return -1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    int new_size = bufSize + 6; // FLAG + A + C + BCC1 + BCC2 + FLAG

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
