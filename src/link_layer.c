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
#define A_T 0x03
#define A_R 0x01
#define C_SET 0x03
#define C_UA 0x07
#define C_DISC 0x0b
#define C_RR(n) 0x05 | (n << 7)  // 0 ou 1
#define C_REJ(n) 0x01 | (n << 7) // 0 ou 1
#define C_INF(n) 0x00 | (n << 6) // 0 ou 1
#define BCC_RCV A ^ C_SET

#define ESCAPE 0x7d
#define ESCAPE_FLAG 0x5e
#define ESCAPE_ESCAPE 0x5d

#define FLAGS_SIZE 4
#define READ_SIZE 1

unsigned char A = A_T;

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
    STOP,
    DATA,
    BCC2_OK,
    FLAG_RCV_END
} state = START;

enum S
{
    ZERO,
    ONE
};
// unsigned char C_received = 0x00;

int alarmEnabled = FALSE;
int alarmCount = 0;

LinkLayer new_layer;
int fd;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
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

unsigned char stateMachine(int fd, unsigned char expected_C)
{
    // printf("\nStarting StateMachine");
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
            // printf("\nFLAG_RCV");
        }
        else
        {
            state = START;
            // printf("\nStart");
        }
        break;

    case FLAG_RCV: // expecting A
        if (byte == A)
        {
            state = A_RCV;
            // printf("\nA_RCV");
        }
        else if (byte == FLAG)
        {
            state = FLAG_RCV;
            // printf("\nFLAG_RCV");
        }
        else
        {
            state = START;
            // printf("\nSTART");
            //  otherwise e stays in the same state FLAG RCV4
        }

        break;

    case A_RCV:
        if (byte == expected_C)
        {
            state = C_RCV;
            // printf("\nC_RCV");
        }
        else if (byte == FLAG)
        {
            state = FLAG_RCV;
            // printf("FLAG_RCV");
        }
        else
        {
            state = START;
            // printf("\nSTART");
        }

        break;

    case C_RCV:
        if (byte == (A ^ expected_C))
        {
            state = BCC_OK;
            // printf("\nBCC_OK");
        }

        else
        {
            // printf("\nSTART");
            state = START;
        }

        break;

    case BCC_OK:
        if (byte == FLAG)
        {
            state = STOP;
            alarm(0);
            // printf("\nSTOP");
        }

        else
        {
            state = START;
            // printf("\nSTART");
        }

        break;

    default:
        // printf("\nERRO ON STATEMACHINE");
        return -1;
        break;
    }
    // printf("\nEnded StateMachine");
    return byte;
    // return C_received;
}

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

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
    new_serial_port_settings.c_cc[VMIN] = 1;  // Blocking read until 1 chars received

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &new_serial_port_settings) < 0)
    {
        perror("\nFailed to set serial attributes: tcsetattr");
        exit(-1);
    }
    printf("\nNew termios structure set");
}

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
    alarmEnabled = FALSE;
    alarmCount = 0;

    switch (new_layer.role)
    {
    case LlTx:; // TRANSMISSOR:

        while (stop == FALSE && alarmCount < new_layer.nRetransmissions)
        {
            // printf("\nLOOP START: %d" , alarmEnabled);
            if (alarmEnabled == FALSE)
            {
                // printf("\n[LOG] Sending SET #%d\n", alarmCount);
                // printf("\nHERE: %d", alarmEnabled);
                // printf("\nalarm nº:%d", alarmCount);
                generateSetTrama(C_SET);
                // int buffer_size = sizeof(BUFFER);
                if (write(fd, BUFFER, SET_SIZE) > 0)
                {
                    printf("\nC_SET enviado\n");
                }

                alarm(new_layer.timeout);
                alarmEnabled = TRUE;
                state = START;
            }

            stateMachine(fd, C_UA);

            if (state == STOP)
            {
                stop = TRUE;
                printf("\nC_UA Received\n");
            }
        }
        state = START;
        alarm(0);
        if (alarmCount >= new_layer.nRetransmissions)
        {
            printf("\nTime Out\n");
            return -1;
        }

        break;

    case LlRx:; // RECEIVER:
        while (state != STOP)
        {
            stateMachine(fd, C_SET);
        }
        state = START;
        //stop = TRUE;

        generateSetTrama(C_UA);
        if (write(fd, BUFFER, SET_SIZE) > 0)
        {
            printf("\nC_UA enviado\n");
            return 0;
        }
    default:
        break;
    }
    return 0;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
void test(unsigned char *trama, unsigned int size)
{

    if (trama[0] == FLAG)
        printf("\nFLAG OK\n");
    if (trama[1] == A)
        printf("\nA OK\n");
    // if (trama[2] == ) printf("\nFLAG OK\n");
    // if (trama[3] == FLAG) printf("\nFLAG OK\n");
 
    if (trama[size - 1] == FLAG)
        printf("\nFLAG 2 OK\n");
}
unsigned char calculateBCC2(const unsigned char *buf, int bufSize)
{
    if (bufSize < 0)
    {
        printf("\nERROR BUF_SIZE: %d\n", bufSize);
    }
    unsigned char BCC2 = 0x00;
    for (unsigned int idx = 0; idx < bufSize; idx++)
    {
        BCC2 = BCC2 ^ buf[idx];
    }
    printf("\nCALCULATE BCC2: %c\n\n\n", BCC2);
    return BCC2;
}

void copyMsg(const unsigned char *buf, int bufSize, unsigned char *new_buf)
{
    if (bufSize < 0)
    {
        printf("\nERROR BUF_SIZE: %d\n", bufSize);
    }

    for (unsigned int idx = 0; idx < bufSize; idx++)
    {
        new_buf[idx + FLAGS_SIZE] = buf[idx];
    }
}

int copyFlag(unsigned char *msg, unsigned int size, unsigned char BCC2)
{
    msg[0] = BUFFER[0];
    msg[1] = BUFFER[1];
    msg[2] = BUFFER[2];
    msg[3] = BUFFER[3];

    msg[size - 2] = BCC2;
    msg[size - 1] = BUFFER[0];
    return 0;
}

unsigned char * stuffing(const unsigned char *buf, unsigned int size, unsigned int * sizeBuf  )
{
    int start = 0;
    unsigned char * Newmsg = (unsigned char *) malloc(sizeof(unsigned char) * (*sizeBuf)); 

    for (int i = 0; i < size; i++)
    {
        if (buf[i] == FLAG)
        {
            (*sizeBuf)++;
            Newmsg = (unsigned char *)realloc(Newmsg, (*sizeBuf));
            Newmsg[start] = ESCAPE;
            Newmsg[start + 1] = ESCAPE_FLAG;

            start = start + 2;
        }

        else if (buf[i] == ESCAPE)
        {
            (*sizeBuf)++;
            Newmsg = (unsigned char *)realloc(Newmsg, (*sizeBuf));
            Newmsg[start] = ESCAPE;
            Newmsg[start + 1] = ESCAPE_ESCAPE;

            start = start + 2;
        }

        else
        {
            Newmsg[start] = buf[i];
            start++;
        }
    }
    return Newmsg;
}

int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char BCC2 = calculateBCC2(buf, bufSize);
    unsigned int size = bufSize;
    unsigned char * stuffed = (unsigned char *) malloc(size * sizeof(unsigned char));
    stuffed = stuffing(buf, bufSize, &size);

    static int n_sequencial = 0;
    
    int stop = FALSE;
    alarmEnabled = FALSE;
    alarmCount = 0;

    unsigned char byte = 0x00;
    
    while (stop == FALSE && alarmCount < new_layer.nRetransmissions)
    {
        if (alarmEnabled == FALSE)
        {
            generateSetTrama(C_INF(n_sequencial));
            unsigned char *msg = (unsigned char *)malloc((size + 6) * sizeof(unsigned char));
            copyFlag(msg, size + 6, BCC2);
            copyMsg(stuffed, size, msg);

            if (write(fd, msg, (size + 6)) > 0)
            {
                printf("\nDATA enviada\n");
            }

            alarm(new_layer.timeout);
            alarmEnabled = TRUE;
            state = START;
        }


        enum STATE last_state = state;
        byte = stateMachine(fd, C_RR(!n_sequencial));



        if (state == STOP)
        {
            stop = TRUE;
            n_sequencial = !n_sequencial;
            printf("\nC_INF Received\n");
        }
    }
    state = START;
    alarm(0);
    if (alarmCount >= new_layer.nRetransmissions)
    {
        printf("\nTime Out llwrite\n");
        return -1;
    }

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
unsigned int destuffing(unsigned char *vet, unsigned int size)
{
    int n = 0;
    for (int i = 0; i < size; i++)
    {
        if (vet[i] == ESCAPE)
        {
            if (vet[i + 1] == ESCAPE_FLAG)
            {
                vet[i] = FLAG;
                for (int j = i + 1; j <= size; j++)
                {
                    vet[j] = vet[j + 1];
                }
                n++;
            }
            if (vet[i + 1] == ESCAPE_ESCAPE)
            {
                for (int j = i + 1; j <= size; j++)
                {
                    vet[j] = vet[j + 1];
                }
                n++;
            }
        }
    }
    return n;
}
int llread(unsigned char *packet)
{
    int n_sequencial = 0;
    unsigned char * msg_rcv = (unsigned char *) malloc(0);
    unsigned int size_rcv = 0;
    state = START;

    while (state != STOP)
    {
        unsigned char byte;
        int bytes_read = read(fd, &byte, READ_SIZE);

        if (bytes_read < 0)
        {
            perror("\nERRO RECEIVING TRAMA\n");
            return -1;
        }

        switch (state)
        {
        case START:
            if (byte == FLAG)
            {
                state = FLAG_RCV;
                printf("\nFLAG\n\n");
            }
            else
                state = START;
            break;


        case FLAG_RCV:
            if (byte == A){
                state = A_RCV;
                printf("\nA_RCV\n\n");
            }

            else if (byte == FLAG){
                state = FLAG_RCV;
                printf("\nFLAG\n\n");
            }
            else
                state = START;
            break;


        case A_RCV:
            if ((byte) == (C_INF(TRUE))){
                state = C_RCV;
                n_sequencial = TRUE;
                printf("\nC_RCV\n\n");
            }
            else if ((byte) == (C_INF(FALSE))){
                state = C_RCV;
                n_sequencial = FALSE;
                printf("\nC_RCV\n\n");
            }
            else if (byte == FLAG){
                state = FLAG_RCV;
                printf("\nFLAG\n\n");
            }
            else
                state = START;
            break;


        case C_RCV:
            if (byte == ((A) ^ (C_INF(n_sequencial)) )){
                printf("\nBCC OK\n\n");
                state = BCC_OK;
            }
            else
                state = START;
            break;


        case BCC_OK:
            if (byte == FLAG){
                printf("\nENDED\n\n");
                state = STOP;
            }

            else
            {
                //printf("\nBYTE\n\n");
                size_rcv++;
                msg_rcv = realloc(msg_rcv, sizeof(unsigned char) * size_rcv);
                msg_rcv[size_rcv - 1] = byte;
            }


        default:
            break;
        }
    }

    unsigned char BCC2_RCV = msg_rcv[size_rcv -1];

    destuffing(msg_rcv, size_rcv-1);
    unsigned char BCC2 = calculateBCC2(msg_rcv, size_rcv-1);

    if (BCC2 == BCC2_RCV)
    {
        printf("\nBCC2 is well received\n\n");
        

        generateSetTrama(C_RR(!n_sequencial));
        if (write(fd, BUFFER, SET_SIZE) < 0){
            printf("\nERRO SENDING C_RR\n");
        }

        for (unsigned int idx = 0; idx < size_rcv - 1; idx++)
        {
            packet[idx] = msg_rcv[idx];
        }
    }
    else
    {
        generateSetTrama(C_REJ(!n_sequencial));
        printf("\nBCC2 failed\n\n");
        // REJ
        if (write(fd, BUFFER, SET_SIZE) < 0)
        {
            printf("\nERRO SENDING C_REJ\n");

        }
        return -1;
    }

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    int stop = FALSE;
    alarmEnabled = FALSE;
    alarmCount = 0;

    state = START;

    switch (new_layer.role)
    {
    case LlTx:

        while (stop == FALSE && alarmCount < new_layer.nRetransmissions)
        {
            if (alarmEnabled == FALSE)
            {
                A = A_T;
                generateSetTrama(C_DISC);
                int buffer_size = sizeof(BUFFER);

                if (write(fd, BUFFER, buffer_size) > 0)
                {
                    printf("\nC_DISC enviado\n");
                }
                A = A_R;
                alarm(new_layer.timeout); // Set alarm to be triggered in 3s
                alarmEnabled = TRUE;
                state = START;
            }

            stateMachine(fd, C_DISC);

            if (state == STOP)
            {
                stop = TRUE;
                printf("\nC_DISC Received\n");
            }
        }

        state = START;
        // printf("\nSTART");
        alarm(0);

        if (alarmCount >= new_layer.nRetransmissions)
        {
            printf("\nTime Out\n");
            return -1;
        }

        A = A_R;
        generateSetTrama(C_UA);
        if (write(fd, BUFFER, SET_SIZE) > 0)
        {
            printf("\nC_UA enviado\n");
        }
        break;

    case LlRx:

        A = A_T;
        while (state != STOP)
        {
            stateMachine(fd, C_DISC);
        }
        printf("Received DISC\n");

        A = A_R;
        generateSetTrama(C_DISC);
        if (write(fd, BUFFER, SET_SIZE) > 0)
        {
            printf("\nC_DISK enviado\n");
        }

        stop = FALSE;
        state = START;
        while (state != STOP)
        {
            stateMachine(fd, C_UA);
        }
        printf("\nReceived C_UA");

        break;

    default:
        break;
    }
    if (tcsetattr(fd, TCSANOW, &old_serial_port_settings) >= 0)
    {
        printf("\nOld termios structure set\n");
    }
    printf("\nFIM DE TRANSMISSAO\n");

    return 0;
}
