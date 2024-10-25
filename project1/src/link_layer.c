// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source
int alarmEnabled = FALSE;
int alarmCount = 0;
bool STOP = FALSE;

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
    int fd = openSerialPort(connectionParameters.serialPort, connectionParameters.baudRate);
    int attempts = 0;
    if (fd < 0)
    {
        printf("\nProblem opening Serial Port\n");
        return -1;
    }
    
    if (connectionParameters.role == LlTx)
    {
        (void) signal(SIGALRM, alarmHandler);
        while (attempts < connectionParameters.nRetransmissions && STOP == FALSE)
        {
            alarm(connectionParameters.timeout);
            alarmEnabled = FALSE;
            while (!alarmEnabled && STOP == FALSE)
            {
                
            }
        }
        

    }
    else if (connectionParameters.role == LlRx)
    {

    }else{
        return -1;
    }
    

    // TODO

    return 1;
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

    int clstat = closeSerialPort();
    return clstat;
}
