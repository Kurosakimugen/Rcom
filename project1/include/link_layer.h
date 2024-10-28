// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#define FLAG  0x7E
#define A_T   0x03 //Sent by Transmitter or answered by Receiver
#define A_R   0x01 //Sent by Receiver or answers by Transmitter
#define C_SET 0x03 //Sent by the transmitter to initiate a connection
#define C_UA  0x07
#define DISC  0x0B
#define ESC   0x7D
#define C_RR0  0xAA
#define C_RR1  0xAB
#define REJ_0 0x54
#define REJ_1 0x55


typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef enum
{
    START,
    FLAG_RCV,
    A_RCV,
    C_RCV,
    BCC_OK,
    STOP,
}DLState;

typedef enum
{
    UNKNOWN,
    ACCEPTED,
    REJECTED
}frameAcknowledgment;

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
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1

void alarmHandler(int signal);

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

int sendSFrame(int fd, unsigned char A, unsigned char C);
int sendUFrame(int fd, unsigned char A, unsigned char C);

bool checkSFrame(int fd, unsigned char A, unsigned char C);  //Implements state machine
bool checkUFrame(int fd, unsigned char A, unsigned char C);
unsigned char checkRRFrame(int fd);

int mountFrame(const unsigned char *buf, int bufSize, unsigned char* frame);




#endif // _LINK_LAYER_H_
