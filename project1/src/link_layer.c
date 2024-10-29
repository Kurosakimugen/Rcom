// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int maxAttempts = 0;
int alarmEnabled = FALSE;
int alarmCount = 0;
int timeout = 0;
LinkLayerRole role;
unsigned char C_IFrame = C_RR0;
extern int fd;

////////////////////////////////////////////////
// ALARM
////////////////////////////////////////////////

void alarmHandler(int signal)
{
    alarmEnabled = FALSE;
    alarmCount++;
    printf("Alarm #%d\n", alarmCount);
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    if (fd < 0)
    {
        printf("\nProblem in llopen opening Serial Port\n");
        return -1;
    }
    maxAttempts = connectionParameters.nRetransmissions;
    timeout = connectionParameters.timeout;
    int attempts = 0;
    
    if (connectionParameters.role == LlTx)
    {
        bool stop = FALSE;
        (void) signal(SIGALRM, alarmHandler);
        while (attempts < maxAttempts && stop == FALSE)
        {
            sendSFrame(fd,A_T,C_SET);
            alarm(timeout);
            alarmEnabled = TRUE;
            stop = checkUFrame(fd,A_R,C_UA);    
            attempts++;
        }
        if (stop == FALSE)
        {
            printf("\tFailed to receive UA in llopen");
            return -1;
        }
        else
        {
            alarm(0);
        }
    }
    else if (connectionParameters.role == LlRx)
    {
        alarmEnabled = FALSE;
        if (!checkSFrame(fd,A_T,C_SET))
            return -1;
    }
    return fd;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    unsigned char* frame = (unsigned char*) malloc(bufSize);
    unsigned char response;
    int frameSize = mountFrame(buf,bufSize,frame);

    int attempts = 0;
    frameAcknowledgment acknowledgment = UNKNOWN;
    
    while (attempts < maxAttempts)
    {
        alarm(timeout);
        alarmEnabled = TRUE;
        write(fd,frame,frameSize);

        response = checkRRFrame(fd);
        if (response == C_RR0 || response == C_RR1)
        {
            acknowledgment = ACCEPTED;
        }
        else if (response == REJ_0 || response == REJ_1)
        {
            acknowledgment = REJECTED;
        }
        else
        {
            acknowledgment = UNKNOWN;
        }


        if (acknowledgment == ACCEPTED)
        {
            C_IFrame = C_IFrame == C_RR0 ? C_RR1 : C_RR0; 
            break;
        }
        attempts++;
    }
    
    free(frame);
    if (acknowledgment == ACCEPTED)
    {
        return frameSize;
    }
    else
    {
        printf("Could not Transmit track in llwrite even after retransmission");
        llclose(fd);
        return -1;
    }
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    unsigned char read_byte;
    unsigned char C;
    I_FrameState status = I_START;
    int index = 0;

    while (status != I_STOP)
    {
        if (read(fd, &read_byte, 1) > 0)
        {
            switch (status)
            {
                case I_START:
                    if (read_byte == FLAG) status = I_FLAG_RCV;
                    break;
                case I_FLAG_RCV:
                    if (read_byte == A_R)
                    {
                        status = I_A_RCV;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = I_FLAG_RCV;
                        break;
                    }
                    status = I_START;
                    break;

                case I_A_RCV:
                    if (read_byte == C_RR0 || read_byte == C_RR1)
                    {
                        status = I_C_RCV;
                        C = read_byte;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = I_FLAG_RCV;
                        break;
                    }
                    else if (read_byte == DISC)
                    {
                        return sendDiscFrame(fd,A_R,DISC) > 0 ? index : -1;
                    }
                    status = I_START;
                    break;
                case I_C_RCV:
                    if (read_byte == (A_T ^ C) )
                    {
                        status = READING_DATA;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = I_FLAG_RCV;
                        break;
                    }
                    status = I_START;
                    break;
                case READING_DATA:
                    if (read_byte == ESC)
                    {
                        status = FOUND_ESC;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        unsigned char packetBcc2 = packet[index--];
                        packet[index] = '\0';

                        unsigned char dataBcc2 = packet[0];
                        for (int i = 1; i < index; i++)
                        {
                            dataBcc2 ^= packet[i];
                        }

                        if (dataBcc2 == packetBcc2)
                        {
                            C_IFrame = C_IFrame == C_RR0 ? C_RR1 : C_RR0;
                            sendUFrame(fd, A_R, C_IFrame);
                            status = I_STOP;
                            return index;
                        }
                        else
                        {
                            printf("\tReceived wrong BCC2\n");
                            unsigned char rej = C_IFrame == C_RR0 ? REJ_0 : REJ_1;
                            sendUFrame(fd,A_R,rej);
                            return -1;
                        }
                    }
                    else
                    {
                        packet[index++] = read_byte; 
                    }
                    break;
                case FOUND_ESC:
                    if (read_byte == ESC_FLAG)
                    {
                        packet[index++] = FLAG;
                    }
                    else if (read_byte == ESC_ESC)
                    {
                        packet[index++] = ESC;
                    }
                    status = READING_DATA;
                    break; 
                case I_STOP:
                    break;
            }

        }
    }
    return index;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)     //TODO show Statistics
{
    bool stop = FALSE;
    int attempts = 0;
    (void) signal(SIGALRM, alarmHandler);
    
    while (attempts < maxAttempts && stop == FALSE)
    {
        sendSFrame(fd,A_T,DISC);
        alarm(timeout);
        alarmEnabled = TRUE;
        stop = checkUFrame(fd, A_R, DISC);

        attempts++;
    }

    if (stop == FALSE)
    {
        printf("Didn't receive Receive confirmation DISC");
        return -1;
    }

    sendUFrame(showStatistics,A_T,C_UA);

    return closeSerialPort();
}



int sendSFrame(int fd, unsigned char A, unsigned char C)
{
    unsigned char set[5] = {FLAG, A, C, (A ^ C), FLAG};
    int res = write(fd,set,5);
    sleep(1);
    return res;
}
int sendUFrame(int fd, unsigned char A, unsigned char C)
{
    return sendSFrame(fd,A,C);
}
int sendDiscFrame(int fd, unsigned char A, unsigned char C)
{
    return sendSFrame(fd,A,C);
}



bool checkSFrame(int fd, unsigned char A, unsigned char C)
{
    S_U_FrameState status = START;
    unsigned char read_byte = 0;
    while (!alarmEnabled && status != STOP)
    {
        if (read(fd, &read_byte, 1) > 0)
        {
            switch (status)
            {
                case START:
                    if (read_byte == FLAG)
                        status = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (read_byte == A)
                    {
                        status = A_RCV;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;

                case A_RCV:
                    if (read_byte == C)
                    {
                        status = C_RCV;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;

                case C_RCV:
                    if (read_byte == (A^C) )
                    {
                        status = BCC_OK;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;
                case BCC_OK:
                    if (read_byte == FLAG)
                    {
                        status = STOP;
                        break;
                    }
                    status = START; 
                    break;
                case STOP:
                    break;
            }
        }
    }
    return status == STOP ? TRUE : FALSE; 
}
bool checkUFrame(int fd, unsigned char A, unsigned char C)
{
    return checkSFrame(fd,A,C);
}

unsigned char checkRRFrame(int fd)
{
    S_U_FrameState status = START;
    unsigned char read_byte;
    unsigned char response;
    while (alarmEnabled && status != STOP)
    {
        if (read(fd, &read_byte, 1) > 0)
        {
            switch (status)
            {
                case START:
                    if (read_byte == FLAG)
                        status = FLAG_RCV;
                    break;
                case FLAG_RCV:
                    if (read_byte == A_R)
                    {
                        status = A_RCV;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;

                case A_RCV:
                    if (read_byte == C_RR0 || read_byte == C_RR1 || read_byte == REJ_0 || read_byte == REJ_1  ) 
                    {
                        status = C_RCV;
                        response = read_byte;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;

                case C_RCV:
                    if (read_byte == (A_R ^ read_byte) )
                    {
                        status = BCC_OK;
                        break;
                    }
                    else if (read_byte == FLAG)
                    {
                        status = FLAG_RCV;
                        break;
                    }
                    status = START;
                    break;
                case BCC_OK:
                    if (read_byte == FLAG)
                    {
                        status = STOP;
                        alarm(0);
                        break;
                    }
                    status = START; 
                    break;
                case STOP:
                    break;
            }

        }
    }
    
    return status == STOP ? response : 0x00;
}

int mountFrame(const unsigned char *buf, int bufSize, unsigned char* frame)
{
    int stuffedBufSize = bufSize + 6;
    frame = realloc(frame,stuffedBufSize);

    frame[0] = FLAG;
    frame[1] = A_T;
    frame[2] = C_IFrame;
    frame[3] = A_T ^ C_IFrame;
    
    unsigned char bcc2 = buf[0];
    int framePos = 4;
    for (int i = 1; i < bufSize; i++, framePos++)
    {
        bcc2 ^= buf[i];

        if (buf[i] == FLAG)
        {
            frame = realloc(frame,++stuffedBufSize);
            frame[framePos++] = ESC;
            frame[framePos]   = 0x5E;
        }
        else if(buf[i] == ESC)
        {
            frame = realloc(frame,++stuffedBufSize);
            frame[framePos++] = ESC;
            frame[framePos]   = 0x5D;
        }
        else
        {
            frame[framePos] = buf[i];
        }
    }
    frame[framePos++] = bcc2;
    frame[framePos++] = FLAG;

    return stuffedBufSize;
}
