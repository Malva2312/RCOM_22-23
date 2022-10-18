// Link layer protocol implementation

#include "link_layer.h"

#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

#define TRANSMISSOR 0
#define RECEIVER 1

#define SET_SIZE 5

#define FLAG 0x7E
#define A 0x03
#define C_SET 0x03
#define BCC_RCV A ^C_SET

#define ESCAPE 0x7D

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
/*typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;*/

    const char *serialPortName = connectionParameters.serialPort;

    int fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

	struct termios old_serial_port_settings;
	struct termios new_serial_port_settings;

    // Save current port settings
	if (tcgetattr(fd, &old_serial_port_settings) < 0)
	{
		perror("Failed to get termios structure: tcgetattr");
		exit(-1);
	}

    // Clear struct for new port settings
    memset(&new_serial_port_settings, 0, sizeof(new_serial_port_settings));

	// setting baud rate to B38400
    new_serial_port_settings.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    new_serial_port_settings.c_iflag = IGNPAR;
    new_serial_port_settings.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    new_serial_port_settings.c_lflag = 0;
    new_serial_port_settings.c_cc[VTIME] = 0; // Inter-character timer unused
    new_serial_port_settings.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &new_serial_port_settings) < 0)
	{
		perror("Failed to set serial attributes: tcsetattr");
		exit(5);
	}
	printf("New termios structure set\n");


    switch (connectionParameters.role)
    {
    case LlTx://TRANSMISSOR:
        unsigned char SET[SET_SIZE];
        SET[0] = FLAG;
        SET[1] = A;
        SET[2] = C_SET;
        SET[3] = BCC_RCV;
        SET[4] = FLAG;

        int Bytes = 0;


        Bytes = write(fd, SET, SET_SIZE);
        
        break;
    
    case LlRx://RECEIVER:

        

        break;
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
    // TODO

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
